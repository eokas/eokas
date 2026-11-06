#include "../NativeWindow.h"
#include "NativeInternal.h"

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <locale.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace eokas
{
    struct NativeWindowState
    {
        ::Display* display = nullptr;
        ::Window xWindow = None;
        XIC ic = nullptr;
        bool closing = false;
        bool mapped = false;
        bool minimized = false;
        bool maximized = false;
        bool grabbed = false;
        bool reportedSize = false;
        int clientWidth = 1;
        int clientHeight = 1;
        NativeCursor cursor = NativeCursor::Arrow;
        bool cursorValid = false;
    };

    namespace
    {
        NativeWindowState& ensureState(NativeWindow& window)
        {
            if (!window.state) {
                window.state = std::make_unique<NativeWindowState>();
            }
            return *window.state;
        }

        NativeWindowState& platform(NativeWindow& window)
        {
            return platform(window);
        }

        const NativeWindowState& platform(const NativeWindow& window)
        {
            return *window.state;
        }

        void detachState(NativeWindowState& platformState)
        {
            platformState.xWindow = None;
            platformState.ic = nullptr;
            platformState.closing = false;
            platformState.mapped = false;
            platformState.grabbed = false;
        }

        constexpr float kWheelStep = 48.0f;
        constexpr unsigned long kMotifDecorations = 1UL << 1;

        struct Connection
        {
            ::Display* display = nullptr;
            XIM im = nullptr;
            ::Atom wmProtocols = None;
            ::Atom wmDelete = None;
            ::Atom netWmName = None;
            ::Atom utf8 = None;
            ::Atom netWmState = None;
            ::Atom maxVert = None;
            ::Atom maxHorz = None;
            ::Atom wmState = None;
            ::Atom frameExtents = None;
            ::Atom workArea = None;
            ::Atom motifHints = None;
            ::Atom windowType = None;
            ::Atom typeNormal = None;
            ::Atom typeUtility = None;
            ::Atom skipTaskbar = None;
            ::Atom activeWindow = None;
            ::Atom gtkTheme = None;
            ::Atom netWmPid = None;
            ::Cursor arrow = None;
            ::Cursor sizeWestEast = None;
            ::Cursor sizeNorthSouth = None;
        };

        ::Atom intern(::Display* display, const char* name)
        {
            return XInternAtom(display, name, False);
        }

        Connection& connection()
        {
            static Connection created = [] {
                Connection value;
                ensureXThreads();
                setlocale(LC_CTYPE, "");
                XSetLocaleModifiers("");
                value.display = XOpenDisplay(nullptr);
                if (value.display == nullptr) {
                    return value;
                }

                ::Display* display = value.display;
                value.wmProtocols = intern(display, "WM_PROTOCOLS");
                value.wmDelete = intern(display, "WM_DELETE_WINDOW");
                value.netWmName = intern(display, "_NET_WM_NAME");
                value.utf8 = intern(display, "UTF8_STRING");
                value.netWmState = intern(display, "_NET_WM_STATE");
                value.maxVert = intern(display, "_NET_WM_STATE_MAXIMIZED_VERT");
                value.maxHorz = intern(display, "_NET_WM_STATE_MAXIMIZED_HORZ");
                value.wmState = intern(display, "WM_STATE");
                value.frameExtents = intern(display, "_NET_FRAME_EXTENTS");
                value.workArea = intern(display, "_NET_WORKAREA");
                value.motifHints = intern(display, "_MOTIF_WM_HINTS");
                value.windowType = intern(display, "_NET_WM_WINDOW_TYPE");
                value.typeNormal = intern(display, "_NET_WM_WINDOW_TYPE_NORMAL");
                value.typeUtility = intern(display, "_NET_WM_WINDOW_TYPE_UTILITY");
                value.skipTaskbar = intern(display, "_NET_WM_STATE_SKIP_TASKBAR");
                value.activeWindow = intern(display, "_NET_ACTIVE_WINDOW");
                value.gtkTheme = intern(display, "_GTK_THEME_VARIANT");
                value.netWmPid = intern(display, "_NET_WM_PID");
                value.arrow = XCreateFontCursor(display, XC_left_ptr);
                value.sizeWestEast = XCreateFontCursor(display, XC_sb_h_double_arrow);
                value.sizeNorthSouth = XCreateFontCursor(display, XC_sb_v_double_arrow);

                value.im = XOpenIM(display, nullptr, nullptr, nullptr);
                return value;
            }();
            return created;
        }

        unsigned dimension(float value)
        {
            if (value < 1.0f) {
                return 1;
            }
            if (value > 65535.0f) {
                return 65535;
            }
            return static_cast<unsigned>(value);
        }

        unsigned long background(::Display* display, int screen)
        {
            XColor color = {};
            color.red = static_cast<unsigned short>(20 * 257);
            color.green = static_cast<unsigned short>(22 * 257);
            color.blue = static_cast<unsigned short>(28 * 257);
            color.flags = DoRed | DoGreen | DoBlue;
            if (XAllocColor(display, DefaultColormap(display, screen), &color) == 0) {
                return BlackPixel(display, screen);
            }
            return color.pixel;
        }

        void replaceUtf8(::Display* display, ::Window window, ::Atom property, ::Atom type, const char* text)
        {
            if (text == nullptr) {
                text = "";
            }
            int length = static_cast<int>(std::strlen(text));
            XChangeProperty(display, window, property, type, 8, PropModeReplace,
                reinterpret_cast<const unsigned char*>(text), length);
        }

        String readUtf8(::Display* display, ::Window window, ::Atom property)
        {
            ::Atom actual = None;
            int format = 0;
            unsigned long count = 0;
            unsigned long remaining = 0;
            unsigned char* data = nullptr;
            int status = XGetWindowProperty(display, window, property, 0, 1024, False, AnyPropertyType,
                &actual, &format, &count, &remaining, &data);
            if (status != Success || data == nullptr || format != 8) {
                if (data != nullptr) {
                    XFree(data);
                }
                return String();
            }
            std::string text(reinterpret_cast<char*>(data), count);
            XFree(data);
            while (!text.empty() && text.back() == '\0') {
                text.pop_back();
            }
            return String(text);
        }

        bool readCardinals(::Display* display, ::Window window, ::Atom property, long* values, unsigned long capacity, unsigned long& count)
        {
            count = 0;
            ::Atom actual = None;
            int format = 0;
            unsigned long remaining = 0;
            unsigned char* data = nullptr;
            int status = XGetWindowProperty(display, window, property, 0, static_cast<long>(capacity), False, XA_CARDINAL,
                &actual, &format, &count, &remaining, &data);
            if (status != Success || data == nullptr || format != 32 || count == 0) {
                if (data != nullptr) {
                    XFree(data);
                }
                count = 0;
                return false;
            }
            auto* longs = reinterpret_cast<long*>(data);
            unsigned long available = count < capacity ? count : capacity;
            for (unsigned long index = 0; index < available; ++index) {
                values[index] = longs[index];
            }
            count = available;
            XFree(data);
            return true;
        }

        void frameExtents(::Display* display, ::Window window, int& left, int& right, int& top, int& bottom)
        {
            left = right = top = bottom = 0;
            long values[4] = {};
            unsigned long count = 0;
            if (!readCardinals(display, window, connection().frameExtents, values, 4, count) || count < 4) {
                return;
            }
            left = static_cast<int>(values[0]);
            right = static_cast<int>(values[1]);
            top = static_cast<int>(values[2]);
            bottom = static_cast<int>(values[3]);
        }

        void sendRoot(::Display* display, ::Window window, ::Atom message, long a, long b, long c, long d)
        {
            XEvent event = {};
            event.xclient.type = ClientMessage;
            event.xclient.window = window;
            event.xclient.message_type = message;
            event.xclient.format = 32;
            event.xclient.data.l[0] = a;
            event.xclient.data.l[1] = b;
            event.xclient.data.l[2] = c;
            event.xclient.data.l[3] = d;
            event.xclient.data.l[4] = 0;
            XSendEvent(display, DefaultRootWindow(display), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &event);
        }

        bool queryIconic(::Display* display, ::Window window)
        {
            ::Atom actual = None;
            int format = 0;
            unsigned long count = 0;
            unsigned long remaining = 0;
            unsigned char* data = nullptr;
            int status = XGetWindowProperty(display, window, connection().wmState, 0, 2, False, AnyPropertyType,
                &actual, &format, &count, &remaining, &data);
            bool iconic = false;
            if (status == Success && data != nullptr && format == 32 && count >= 1) {
                iconic = reinterpret_cast<long*>(data)[0] == IconicState;
            }
            if (data != nullptr) {
                XFree(data);
            }
            return iconic;
        }

        bool queryMaximized(::Display* display, ::Window window)
        {
            ::Atom actual = None;
            int format = 0;
            unsigned long count = 0;
            unsigned long remaining = 0;
            unsigned char* data = nullptr;
            int status = XGetWindowProperty(display, window, connection().netWmState, 0, 64, False, XA_ATOM,
                &actual, &format, &count, &remaining, &data);
            bool vertical = false;
            bool horizontal = false;
            if (status == Success && data != nullptr && format == 32) {
                auto* values = reinterpret_cast<long*>(data);
                ::Atom maxVert = connection().maxVert;
                ::Atom maxHorz = connection().maxHorz;
                for (unsigned long index = 0; index < count; ++index) {
                    ::Atom value = static_cast<::Atom>(values[index]);
                    if (value == maxVert) vertical = true;
                    if (value == maxHorz) horizontal = true;
                }
            }
            if (data != nullptr) {
                XFree(data);
            }
            return vertical || horizontal;
        }

        bool extendedKey(::KeySym keysym)
        {
            if (IsKeypadKey(keysym)) {
                return true;
            }
            switch (keysym) {
            case XK_Left:
            case XK_Right:
            case XK_Up:
            case XK_Down:
            case XK_Insert:
            case XK_Delete:
            case XK_Home:
            case XK_End:
            case XK_Page_Up:
            case XK_Page_Down:
                return true;
            default:
                return false;
            }
        }

        void emitChars(NativeWindow* owner, const char* bytes, int length)
        {
            if (owner == nullptr || !owner->onKeyChar || bytes == nullptr || length <= 0) {
                return;
            }
            const auto* data = reinterpret_cast<const unsigned char*>(bytes);
            int index = 0;
            while (index < length) {
                unsigned char lead = data[index];
                uint32_t codepoint = 0;
                int need = 1;
                if ((lead & 0x80) == 0) {
                    codepoint = lead;
                } else if ((lead & 0xE0) == 0xC0) {
                    codepoint = lead & 0x1F;
                    need = 2;
                } else if ((lead & 0xF0) == 0xE0) {
                    codepoint = lead & 0x0F;
                    need = 3;
                } else if ((lead & 0xF8) == 0xF0) {
                    codepoint = lead & 0x07;
                    need = 4;
                } else {
                    index += 1;
                    continue;
                }
                if (index + need > length) {
                    break;
                }
                bool valid = true;
                for (int extra = 1; extra < need; ++extra) {
                    unsigned char next = data[index + extra];
                    if ((next & 0xC0) != 0x80) {
                        valid = false;
                        break;
                    }
                    codepoint = (codepoint << 6) | (next & 0x3F);
                }
                if (!valid) {
                    index += 1;
                    continue;
                }
                index += need;
                owner->onKeyChar(codepoint);
            }
        }

        Bool matchWindow(::Display*, XEvent* event, XPointer arg)
        {
            auto window = *reinterpret_cast<::Window*>(arg);
            return event->xany.window == window ? True : False;
        }

        void drain(::Display* display, ::Window window)
        {
            if (display == nullptr || window == None) {
                return;
            }
            XEvent event;
            while (XCheckIfEvent(display, &event, &matchWindow, reinterpret_cast<XPointer>(&window))) {
            }
        }

        bool inputStyleSupported(XIM im)
        {
            if (im == nullptr) {
                return false;
            }
            XIMStyles* styles = nullptr;
            char* missing = XGetIMValues(im, XNQueryInputStyle, &styles, nullptr);
            if (missing != nullptr || styles == nullptr) {
                if (styles != nullptr) {
                    XFree(styles);
                }
                return false;
            }
            XIMStyle wanted = XIMPreeditNothing | XIMStatusNothing;
            bool supported = false;
            for (int index = 0; index < styles->count_styles; ++index) {
                if (styles->supported_styles[index] == wanted) {
                    supported = true;
                    break;
                }
            }
            XFree(styles);
            return supported;
        }

        void applyCursor(NativeWindow& window, float x, float y)
        {
        NativeCursor next = NativeCursor::Arrow;
        if (window.onCursor) {
            NativeCursor requested = NativeCursor::Arrow;
            if (window.onCursor(x, y, requested)) {
                next = requested;
            }
        }
        NativeWindowState& platform = platform(window);
        if (platform.cursorValid && platform.cursor == next) {
            return;
        }
        platform.cursor = next;
        platform.cursorValid = true;
        const Connection& current = connection();
        ::Cursor shape = current.arrow;
        if (next == NativeCursor::SizeWestEast) {
            shape = current.sizeWestEast;
        } else if (next == NativeCursor::SizeNorthSouth) {
            shape = current.sizeNorthSouth;
        }
        if (shape != None) {
            XDefineCursor(platform.display, platform.xWindow, shape);
        }
        }

        void handleEvent(NativeWindow& window, XEvent& event)
        {
        if (XFilterEvent(&event, None)) {
            return;
        }
        const Connection& current = connection();

        switch (event.type) {
        case ClientMessage:
            if (event.xclient.message_type == current.wmProtocols
                && static_cast<::Atom>(event.xclient.data.l[0]) == current.wmDelete) {
                if (window.onCloseQuery && !window.onCloseQuery()) {
                    return;
                }
                window.destroy();
            }
            return;
        case DestroyNotify:
            return;
        case MapNotify:
            platform(window).mapped = true;
            return;
        case UnmapNotify:
            platform(window).mapped = false;
            return;
        case PropertyNotify:
            if (event.xproperty.atom == current.wmState) {
                platform(window).minimized = queryIconic(platform(window).display, platform(window).xWindow);
            } else if (event.xproperty.atom == current.netWmState) {
                platform(window).maximized = queryMaximized(platform(window).display, platform(window).xWindow);
            }
            return;
        case ConfigureNotify: {
            int width = event.xconfigure.width;
            int height = event.xconfigure.height;
            if (width < 0) width = 0;
            if (height < 0) height = 0;
            NativeWindowState& platform = platform(window);
            if (!platform.reportedSize || width != platform.clientWidth || height != platform.clientHeight) {
                platform.reportedSize = true;
                platform.clientWidth = width;
                platform.clientHeight = height;
                if (window.onClientSize) {
                    window.onClientSize(static_cast<float>(width), static_cast<float>(height));
                }
            }
            return;
        }
        case MotionNotify: {
            float x = static_cast<float>(event.xmotion.x);
            float y = static_cast<float>(event.xmotion.y);
            if (window.onMouseMove) {
                window.onMouseMove(x, y);
            }
            applyCursor(window, x, y);
            return;
        }
        case EnterNotify: {
            float x = static_cast<float>(event.xcrossing.x);
            float y = static_cast<float>(event.xcrossing.y);
            if (window.onMouseMove) {
                window.onMouseMove(x, y);
            }
            applyCursor(window, x, y);
            return;
        }
        case ButtonPress: {
            unsigned int button = event.xbutton.button;
            float x = static_cast<float>(event.xbutton.x);
            float y = static_cast<float>(event.xbutton.y);
            bool shift = (event.xbutton.state & ShiftMask) != 0;
            float deltaX = 0.0f;
            float deltaY = 0.0f;
            NativeWindowState& platform = platform(window);
            if (button == 4) {
                deltaY = -kWheelStep;
            } else if (button == 5) {
                deltaY = kWheelStep;
            } else if (button == 6) {
                deltaX = -kWheelStep;
            } else if (button == 7) {
                deltaX = kWheelStep;
            } else {
                if (!platform.grabbed) {
                    platform.grabbed = XGrabPointer(platform.display, platform.xWindow, True,
                        ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                        GrabModeAsync, GrabModeAsync, None, None, event.xbutton.time) == GrabSuccess;
                }
                if (window.onMouseDown) {
                    window.onMouseDown(x, y, transferMouseButton(static_cast<int>(button)));
                }
                applyCursor(window, x, y);
                return;
            }
            if (shift && deltaX == 0.0f) {
                deltaX = deltaY;
                deltaY = 0.0f;
            }
            if (window.onMouseWheel) {
                window.onMouseWheel(x, y, deltaX, deltaY);
            }
            return;
        }
        case ButtonRelease: {
            unsigned int button = event.xbutton.button;
            if (button >= 4 && button <= 7) {
                return;
            }
            if (window.onMouseUp) {
                window.onMouseUp(static_cast<float>(event.xbutton.x), static_cast<float>(event.xbutton.y), transferMouseButton(static_cast<int>(button)));
            }
            NativeWindowState& platform = platform(window);
            if (platform.grabbed) {
                XUngrabPointer(platform.display, event.xbutton.time);
                platform.grabbed = false;
            }
            return;
        }
        case KeyPress: {
            ::KeySym typed = NoSymbol;
            char stack[64];
            std::vector<char> storage;
            int length = 0;
            ::Status status = 0;
            char* chars = stack;
            NativeWindowState& platform = platform(window);
            if (platform.ic != nullptr) {
                length = Xutf8LookupString(platform.ic, &event.xkey, stack, static_cast<int>(sizeof(stack)) - 1, &typed, &status);
                if (status == XBufferOverflow && length > 0) {
                    storage.resize(static_cast<size_t>(length) + 1);
                    length = Xutf8LookupString(platform.ic, &event.xkey, storage.data(), length, &typed, &status);
                    chars = storage.data();
                }
            } else {
                length = XLookupString(&event.xkey, stack, static_cast<int>(sizeof(stack)) - 1, &typed, nullptr);
            }
            if (length < 0) {
                length = 0;
            }
            ::KeySym keysym = XLookupKeysym(&event.xkey, 0);
            if (keysym == NoSymbol) {
                keysym = typed;
            }
            bool ctrl = (event.xkey.state & ControlMask) != 0;
            bool shift = (event.xkey.state & ShiftMask) != 0;
            bool alt = (event.xkey.state & Mod1Mask) != 0;
            if (keysym != NoSymbol && window.onKeyPress) {
                NativeKeyCode keyCode = transferKeyCode(static_cast<int>(keysym), false);
                window.onKeyPress(keyCode, extendedKey(keysym), ctrl, shift, alt, false);
            }
            emitChars(&window, chars, length);
            return;
        }
        case KeyRelease: {
            bool ctrl = (event.xkey.state & ControlMask) != 0;
            bool shift = (event.xkey.state & ShiftMask) != 0;
            bool alt = (event.xkey.state & Mod1Mask) != 0;
            ::KeySym keysym = XLookupKeysym(&event.xkey, 0);
            if (keysym != NoSymbol && window.onKeyRelease) {
                window.onKeyRelease(transferKeyCode(static_cast<int>(keysym), false), extendedKey(keysym), ctrl, shift, alt);
            }
            return;
        }
        case FocusIn:
            if (platform(window).ic != nullptr) {
                XSetICFocus(platform(window).ic);
            }
            return;
        case FocusOut:
            if (platform(window).ic != nullptr) {
                XUnsetICFocus(platform(window).ic);
            }
            return;
        default:
            return;
        }
        }
    }

    NativeWindow::NativeWindow() = default;

    NativeWindow::~NativeWindow()
    {
        this->destroy();
        state.reset();
    }

    bool NativeWindow::create(const String& title, float width, float height, NativeWindowStyle style)
    {
        if (state != nullptr && platform(*this).xWindow != None) {
            return true;
        }
        Connection& current = connection();
        ::Display* display = current.display;
        if (display == nullptr) {
            return false;
        }
        platform(*this).display = display;

        bool borderless = style == NativeWindowStyle::Borderless;
        unsigned clientWidth = dimension(width);
        unsigned clientHeight = dimension(height);
        int screen = DefaultScreen(display);
        XSetWindowAttributes attributes = {};
        attributes.event_mask =
            ExposureMask
            | StructureNotifyMask
            | KeyPressMask
            | ButtonPressMask
            | ButtonReleaseMask
            | PointerMotionMask
            | EnterWindowMask
            | FocusChangeMask
            | PropertyChangeMask;
        attributes.background_pixel = background(display, screen);
        attributes.border_pixel = attributes.background_pixel;

        int origin = borderless ? 0 : 0;
        ::Window window = XCreateWindow(
            display,
            RootWindow(display, screen),
            origin,
            origin,
            clientWidth,
            clientHeight,
            0,
            CopyFromParent,
            InputOutput,
            CopyFromParent,
            CWEventMask | CWBackPixel | CWBorderPixel,
            &attributes);
        if (window == None) {
            return false;
        }

        XSetWMProtocols(display, window, &current.wmDelete, 1);
        const char* utf8 = title.cstr();
        replaceUtf8(display, window, current.netWmName, current.utf8, utf8);
        replaceUtf8(display, window, XA_WM_NAME, current.utf8, utf8);
        replaceUtf8(display, window, XA_WM_ICON_NAME, current.utf8, utf8);

        XClassHint hint = {};
        hint.res_name = const_cast<char*>("eokas");
        hint.res_class = const_cast<char*>("Eokas");
        XSetClassHint(display, window, &hint);

        long pid = static_cast<long>(::getpid());
        XChangeProperty(display, window, current.netWmPid, XA_CARDINAL, 32, PropModeReplace,
            reinterpret_cast<unsigned char*>(&pid), 1);

        long type = static_cast<long>(borderless ? current.typeUtility : current.typeNormal);
        XChangeProperty(display, window, current.windowType, XA_ATOM, 32, PropModeReplace,
            reinterpret_cast<unsigned char*>(&type), 1);

        if (borderless) {
            long motif[5] = {};
            motif[0] = static_cast<long>(kMotifDecorations);
            XChangeProperty(display, window, current.motifHints, current.motifHints, 32, PropModeReplace,
                reinterpret_cast<unsigned char*>(motif), 5);
            long skip = static_cast<long>(current.skipTaskbar);
            XChangeProperty(display, window, current.netWmState, XA_ATOM, 32, PropModeReplace,
                reinterpret_cast<unsigned char*>(&skip), 1);
        } else {
            const char dark[] = "dark";
            XChangeProperty(display, window, current.gtkTheme, current.utf8, 8, PropModeReplace,
                reinterpret_cast<const unsigned char*>(dark), 4);
        }

        XSizeHints size = {};
        size.flags = PMinSize | PBaseSize;
        size.min_width = 1;
        size.min_height = 1;
        size.base_width = static_cast<int>(clientWidth);
        size.base_height = static_cast<int>(clientHeight);
        if (borderless) {
            size.flags |= USPosition | PPosition;
            size.x = 0;
            size.y = 0;
        }
        XSetWMNormalHints(display, window, &size);

        if (current.arrow != None) {
            XDefineCursor(display, window, current.arrow);
        }
        if (inputStyleSupported(current.im)) {
            platform(*this).ic = XCreateIC(current.im,
                XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                XNClientWindow, window,
                XNFocusWindow, window,
                nullptr);
        }

        platform(*this).xWindow = window;
        platform(*this).closing = false;
        platform(*this).mapped = false;
        platform(*this).minimized = false;
        platform(*this).maximized = false;
        platform(*this).grabbed = false;
        platform(*this).reportedSize = false;
        platform(*this).clientWidth = static_cast<int>(clientWidth);
        platform(*this).clientHeight = static_cast<int>(clientHeight);
        platform(*this).cursor = NativeCursor::Arrow;
        platform(*this).cursorValid = true;
        XFlush(display);
        return true;
    }

    void NativeWindow::destroy()
    {
        if (!state) {
            return;
        }
        if (platform(*this).xWindow == None || platform(*this).closing) {
            return;
        }
        platform(*this).closing = true;
        if (onClose) {
            auto callback = std::move(onClose);
            callback();
        }
        if (platform(*this).xWindow == None) {
            return;
        }

        ::Display* display = platform(*this).display;
        ::Window window = platform(*this).xWindow;
        XIC ic = platform(*this).ic;
        if (platform(*this).grabbed && display != nullptr) {
            XUngrabPointer(display, CurrentTime);
            platform(*this).grabbed = false;
        }
        platform(*this).ic = nullptr;
        detachState(platform(*this));
        if (ic != nullptr) {
            XDestroyIC(ic);
        }
        if (display != nullptr && window != None) {
            XDestroyWindow(display, window);
            XSync(display, False);
            drain(display, window);
        }
    }

    void NativeWindow::show(bool activate)
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        ::Display* display = platform(*this).display;
        XMapWindow(display, platform(*this).xWindow);
        platform(*this).mapped = true;
        platform(*this).minimized = false;
        if (activate) {
            XRaiseWindow(display, platform(*this).xWindow);
            const Connection& current = connection();
            sendRoot(display, platform(*this).xWindow, current.activeWindow, 1, CurrentTime, 0, 0);
            if (platform(*this).ic != nullptr) {
                XSetICFocus(platform(*this).ic);
            }
        }
        XFlush(display);
    }

    void NativeWindow::hide()
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        XUnmapWindow(platform(*this).display, platform(*this).xWindow);
        platform(*this).mapped = false;
        platform(*this).minimized = false;
        XFlush(platform(*this).display);
    }

    void NativeWindow::close()
    {
        this->destroy();
    }

    void NativeWindow::setTitle(const String& title)
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        const Connection& current = connection();
        const char* utf8 = title.cstr();
        replaceUtf8(platform(*this).display, platform(*this).xWindow, current.netWmName, current.utf8, utf8);
        replaceUtf8(platform(*this).display, platform(*this).xWindow, XA_WM_NAME, current.utf8, utf8);
        replaceUtf8(platform(*this).display, platform(*this).xWindow, XA_WM_ICON_NAME, current.utf8, utf8);
        XFlush(platform(*this).display);
    }

    String NativeWindow::title() const
    {
        if (platform(*this).xWindow == None) {
            return String();
        }
        const Connection& current = connection();
        String text = readUtf8(platform(*this).display, platform(*this).xWindow, current.netWmName);
        if (!text.isEmpty()) {
            return text;
        }
        return readUtf8(platform(*this).display, platform(*this).xWindow, XA_WM_NAME);
    }

    void NativeWindow::setClientSize(float width, float height)
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        XResizeWindow(platform(*this).display, platform(*this).xWindow, dimension(width), dimension(height));
        XFlush(platform(*this).display);
    }

    Vector2 NativeWindow::clientSize() const
    {
        if (platform(*this).xWindow == None) {
            return Vector2();
        }
        XWindowAttributes attributes = {};
        if (XGetWindowAttributes(platform(*this).display, platform(*this).xWindow, &attributes) == 0) {
            return Vector2(static_cast<float>(platform(*this).clientWidth), static_cast<float>(platform(*this).clientHeight));
        }
        return Vector2(static_cast<float>(attributes.width), static_cast<float>(attributes.height));
    }

    void NativeWindow::setPosition(float x, float y)
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        ::Display* display = platform(*this).display;
        XSync(display, False);
        int left = 0;
        int right = 0;
        int top = 0;
        int bottom = 0;
        frameExtents(display, platform(*this).xWindow, left, right, top, bottom);
        (void)right;
        (void)bottom;
        XMoveWindow(display, platform(*this).xWindow, static_cast<int>(x) + left, static_cast<int>(y) + top);
        XFlush(display);
    }

    Vector2 NativeWindow::position() const
    {
        if (platform(*this).xWindow == None) {
            return Vector2();
        }
        ::Display* display = platform(*this).display;
        ::Window child = None;
        int x = 0;
        int y = 0;
        if (XTranslateCoordinates(display, platform(*this).xWindow, DefaultRootWindow(display), 0, 0, &x, &y, &child) == 0) {
            return Vector2();
        }
        int left = 0;
        int right = 0;
        int top = 0;
        int bottom = 0;
        frameExtents(display, platform(*this).xWindow, left, right, top, bottom);
        (void)right;
        (void)bottom;
        return Vector2(static_cast<float>(x - left), static_cast<float>(y - top));
    }

    void NativeWindow::center()
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        ::Display* display = platform(*this).display;
        XSync(display, False);
        int screen = DefaultScreen(display);
        int workX = 0;
        int workY = 0;
        int workW = DisplayWidth(display, screen);
        int workH = DisplayHeight(display, screen);
        long work[4] = {};
        unsigned long count = 0;
        if (readCardinals(display, DefaultRootWindow(display), connection().workArea, work, 4, count) && count >= 4) {
            workX = static_cast<int>(work[0]);
            workY = static_cast<int>(work[1]);
            workW = static_cast<int>(work[2]);
            workH = static_cast<int>(work[3]);
        }
        int left = 0;
        int right = 0;
        int top = 0;
        int bottom = 0;
        frameExtents(display, platform(*this).xWindow, left, right, top, bottom);
        Vector2 client = this->clientSize();
        int frameW = static_cast<int>(client.x) + left + right;
        int frameH = static_cast<int>(client.y) + top + bottom;
        int x = workX + (workW - frameW) / 2;
        int y = workY + (workH - frameH) / 2;
        XMoveWindow(display, platform(*this).xWindow, x + left, y + top);
        XFlush(display);
    }

    void NativeWindow::focus()
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        ::Display* display = platform(*this).display;
        XRaiseWindow(display, platform(*this).xWindow);
        sendRoot(display, platform(*this).xWindow, connection().activeWindow, 1, CurrentTime, 0, 0);
        XSetInputFocus(display, platform(*this).xWindow, RevertToParent, CurrentTime);
        if (platform(*this).ic != nullptr) {
            XSetICFocus(platform(*this).ic);
        }
        XFlush(display);
    }

    void NativeWindow::minimize()
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        platform(*this).minimized = true;
        platform(*this).maximized = false;
        XIconifyWindow(platform(*this).display, platform(*this).xWindow, DefaultScreen(platform(*this).display));
        XFlush(platform(*this).display);
    }

    void NativeWindow::maximize()
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        platform(*this).maximized = true;
        platform(*this).minimized = false;
        platform(*this).mapped = true;
        const Connection& current = connection();
        XMapWindow(platform(*this).display, platform(*this).xWindow);
        sendRoot(platform(*this).display, platform(*this).xWindow, current.netWmState, 1, static_cast<long>(current.maxVert), static_cast<long>(current.maxHorz), 1);
        XFlush(platform(*this).display);
    }

    void NativeWindow::restore()
    {
        if (platform(*this).xWindow == None) {
            return;
        }
        bool wasMinimized = platform(*this).minimized;
        bool wasMaximized = platform(*this).maximized;
        platform(*this).minimized = false;
        platform(*this).maximized = false;
        platform(*this).mapped = true;
        ::Display* display = platform(*this).display;
        if (wasMinimized) {
            XMapWindow(display, platform(*this).xWindow);
        }
        if (wasMaximized) {
            const Connection& current = connection();
            sendRoot(display, platform(*this).xWindow, current.netWmState, 0, static_cast<long>(current.maxVert), static_cast<long>(current.maxHorz), 1);
        }
        XFlush(display);
    }

    bool NativeWindow::isOpen() const
    {
        return platform(*this).xWindow != None;
    }

    bool NativeWindow::isVisible() const
    {
        return platform(*this).xWindow != None && platform(*this).mapped && !platform(*this).minimized;
    }

    bool NativeWindow::isMinimized() const
    {
        return platform(*this).xWindow != None && platform(*this).minimized;
    }

    bool NativeWindow::isMaximized() const
    {
        return platform(*this).xWindow != None && platform(*this).maximized;
    }

    void NativeWindow::pumpMessages()
    {
        if (platform(*this).xWindow == None || platform(*this).display == nullptr) {
            return;
        }
        ::Window window = platform(*this).xWindow;
        ::Display* display = platform(*this).display;
        XEvent event;
        while (XCheckIfEvent(display, &event, &matchWindow, reinterpret_cast<XPointer>(&window))) {
            handleEvent(*this, event);
            if (platform(*this).xWindow != window) {
                break;
            }
        }
    }

    void* NativeWindow::handle() const
    {
        if (platform(*this).xWindow == None) {
            return nullptr;
        }
        return reinterpret_cast<void*>(static_cast<uintptr_t>(platform(*this).xWindow));
    }

    void* NativeWindow::display() const
    {
        if (platform(*this).display != nullptr) {
            return platform(*this).display;
        }
        return connection().display;
    }

}
