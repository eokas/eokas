#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <sdkddkver.h>
#include "../NativeWindow.h"

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

#include <string>

#pragma comment(lib, "dwmapi.lib")

namespace eokas
{
    struct NativeWindowState
    {
        void* hwnd = nullptr;
        unsigned short pendingHigh = 0;
        unsigned long style = 0;
        unsigned long exStyle = 0;
    };

    namespace
    {
        NativeWindowState& platform(NativeWindow& window)
        {
            if (!window.state) {
                window.state = std::make_unique<NativeWindowState>();
            }
            return *window.state;
        }

        const NativeWindowState& platform(const NativeWindow& window)
        {
            static const NativeWindowState empty{};
            return window.state ? *window.state : empty;
        }

        HWND hwnd(const NativeWindow& window)
        {
            HWND value = (HWND)platform(window).hwnd;
            return value && IsWindow(value) ? value : nullptr;
        }

        void detachState(NativeWindowState& state)
        {
            state.hwnd = nullptr;
            state.pendingHigh = 0;
        }

        constexpr DWORD kStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
        constexpr DWORD kExStyle = 0;
        constexpr const wchar_t* kClassName = L"eokas.NativeWindow";

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif

        void enablePerMonitorDpi()
        {
            static bool done = false;
            if (done) {
                return;
            }
            done = true;
            SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }

        UINT windowDpi(HWND hwnd)
        {
            UINT dpi = hwnd ? GetDpiForWindow(hwnd) : GetDpiForSystem();
            return dpi ? dpi : 96;
        }

        void outerSize(HWND hwnd, float width, float height, DWORD style, DWORD exStyle, int& outerWidth, int& outerHeight)
        {
            int clientWidth = width < 0.0f ? 0 : (int)width;
            int clientHeight = height < 0.0f ? 0 : (int)height;
            if ((style & WS_POPUP) != 0 && (style & WS_CAPTION) == 0) {
                outerWidth = clientWidth;
                outerHeight = clientHeight;
                return;
            }
            RECT rect = { 0, 0, clientWidth, clientHeight };
            AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, windowDpi(hwnd));
            outerWidth = rect.right - rect.left;
            outerHeight = rect.bottom - rect.top;
        }

        void useDarkCaption(HWND hwnd)
        {
            BOOL enabled = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &enabled, sizeof(enabled));
            COLORREF caption = RGB(20, 22, 28);
            DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
            COLORREF text = RGB(236, 238, 243);
            DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &text, sizeof(text));
        }

        std::wstring titleWide(const String& title)
        {
            const char* utf8 = title.cstr();
            return utf8 ? String::utf8ToUnicode(utf8, false) : std::wstring();
        }

        int platformMouseButton(UINT message, WPARAM wParam)
        {
            switch (message) {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONDBLCLK:
            case WM_LBUTTONUP:
                return VK_LBUTTON;
            case WM_RBUTTONDOWN:
            case WM_RBUTTONDBLCLK:
            case WM_RBUTTONUP:
                return VK_RBUTTON;
            case WM_MBUTTONDOWN:
            case WM_MBUTTONDBLCLK:
            case WM_MBUTTONUP:
                return VK_MBUTTON;
            case WM_XBUTTONDOWN:
            case WM_XBUTTONDBLCLK:
            case WM_XBUTTONUP:
                return GET_XBUTTON_WPARAM(wParam) == XBUTTON2 ? VK_XBUTTON2 : VK_XBUTTON1;
            default:
                return 0;
            }
        }

        bool applyCursor(NativeWindow& window, HWND hwnd)
        {
            if (!window.onCursor) {
                return false;
            }
            POINT point;
            if (!GetCursorPos(&point) || !ScreenToClient(hwnd, &point)) {
                return false;
            }
            NativeCursor cursor = NativeCursor::Arrow;
            if (!window.onCursor((float)point.x, (float)point.y, cursor)) {
                return false;
            }
            LPCWSTR shape = IDC_ARROW;
            if (cursor == NativeCursor::SizeWestEast) {
                shape = IDC_SIZEWE;
            } else if (cursor == NativeCursor::SizeNorthSouth) {
                shape = IDC_SIZENS;
            }
            SetCursor(LoadCursorW(nullptr, shape));
            return true;
        }

        bool handleChar(NativeWindow& window, wchar_t unit)
        {
            NativeWindowState& state = platform(window);
            uint32_t codepoint = 0;
            if (unit >= 0xD800 && unit <= 0xDBFF) {
                state.pendingHigh = unit;
                return true;
            }
            if (unit >= 0xDC00 && unit <= 0xDFFF && state.pendingHigh != 0) {
                wchar_t high = (wchar_t)state.pendingHigh;
                state.pendingHigh = 0;
                codepoint = 0x10000 + (((uint32_t)high - 0xD800) << 10) + ((uint32_t)unit - 0xDC00);
            } else {
                state.pendingHigh = 0;
                codepoint = (uint32_t)unit;
            }
            return window.onKeyChar && window.onKeyChar(codepoint);
        }

        LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (message == WM_NCCREATE) {
                CREATESTRUCTW* created = reinterpret_cast<CREATESTRUCTW*>(lParam);
                auto* window = reinterpret_cast<NativeWindow*>(created->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
                if (window != nullptr) {
                    platform(*window).hwnd = (void*)hwnd;
                }
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }

            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_PAINT) {
                PAINTSTRUCT paint;
                BeginPaint(hwnd, &paint);
                EndPaint(hwnd, &paint);
                return 0;
            }

            auto* window = reinterpret_cast<NativeWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (window == nullptr) {
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }

            switch (message) {
            case WM_CLOSE:
                if (window->onCloseQuery && !window->onCloseQuery()) {
                    return 0;
                }
                window->destroy();
                return 0;
            case WM_DESTROY:
                detachState(platform(*window));
                return 0;
            case WM_DPICHANGED: {
                RECT* suggested = reinterpret_cast<RECT*>(lParam);
                if (suggested != nullptr) {
                    SetWindowPos(hwnd, nullptr,
                        suggested->left,
                        suggested->top,
                        suggested->right - suggested->left,
                        suggested->bottom - suggested->top,
                        SWP_NOZORDER | SWP_NOACTIVATE);
                }
                return 0;
            }
            case WM_SIZE:
                if (wParam != SIZE_MINIMIZED && window->onClientSize) {
                    window->onClientSize((float)LOWORD(lParam), (float)HIWORD(lParam));
                }
                return 0;
            case WM_MOUSEMOVE:
                if (window->onMouseMove) {
                    window->onMouseMove((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
                }
                return 0;
            case WM_LBUTTONDOWN:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONDBLCLK:
                SetCapture(hwnd);
                if (window->onMouseDown) {
                    NativeMouseButton button = transferMouseButton(platformMouseButton(message, wParam));
                    window->onMouseDown((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), button);
                }
                return (message == WM_XBUTTONDOWN || message == WM_XBUTTONDBLCLK) ? TRUE : 0;
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
                if (window->onMouseUp) {
                    NativeMouseButton button = transferMouseButton(platformMouseButton(message, wParam));
                    window->onMouseUp((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), button);
                }
                if (GetCapture() == hwnd) {
                    ReleaseCapture();
                }
                return message == WM_XBUTTONUP ? TRUE : 0;
            case WM_SETCURSOR:
                if (LOWORD(lParam) == HTCLIENT && applyCursor(*window, hwnd)) {
                    return TRUE;
                }
                return DefWindowProcW(hwnd, message, wParam, lParam);
            case WM_MOUSEWHEEL: {
                POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(hwnd, &point);
                float step = -(float)GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f * 48.0f;
                if (window->onMouseWheel) {
                    if ((GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT) != 0) {
                        window->onMouseWheel((float)point.x, (float)point.y, step, 0.0f);
                    } else {
                        window->onMouseWheel((float)point.x, (float)point.y, 0.0f, step);
                    }
                }
                return 0;
            }
            case WM_KEYDOWN:
                if (window->onKeyPress) {
                    bool extended = (lParam & (1 << 24)) != 0;
                    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                    bool repeat = (lParam & (1 << 30)) != 0;
                    if (window->onKeyPress(transferKeyCode((int)wParam, extended), extended, ctrl, shift, alt, repeat)) {
                        return 0;
                    }
                }
                return DefWindowProcW(hwnd, message, wParam, lParam);
            case WM_KEYUP:
                if (window->onKeyRelease) {
                    bool extended = (lParam & (1 << 24)) != 0;
                    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                    if (window->onKeyRelease(transferKeyCode((int)wParam, extended), extended, ctrl, shift, alt)) {
                        return 0;
                    }
                }
                return DefWindowProcW(hwnd, message, wParam, lParam);
            case WM_CHAR:
                if (handleChar(*window, (wchar_t)wParam)) {
                    return 0;
                }
                return DefWindowProcW(hwnd, message, wParam, lParam);
            default:
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }
        }

        bool registerWindowClass(HINSTANCE instance)
        {
            static bool registered = false;
            if (registered) {
                return true;
            }

            WNDCLASSEXW windowClass = {};
            windowClass.cbSize = sizeof(windowClass);
            windowClass.style = CS_DBLCLKS;
            windowClass.lpfnWndProc = &windowProc;
            windowClass.hInstance = instance;
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
            windowClass.lpszClassName = kClassName;

            ATOM atom = RegisterClassExW(&windowClass);
            if (atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                return false;
            }
            registered = true;
            return true;
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
        if (hwnd(*this)) {
            return true;
        }

        enablePerMonitorDpi();

        HINSTANCE instance = GetModuleHandleW(nullptr);
        if (!registerWindowClass(instance)) {
            return false;
        }

        NativeWindowState& state = platform(*this);
        if (style == NativeWindowStyle::Borderless) {
            state.style = WS_POPUP | WS_CLIPCHILDREN;
            state.exStyle = WS_EX_TOOLWINDOW;
        } else {
            state.style = kStyle;
            state.exStyle = kExStyle;
        }

        int outerWidth = 0;
        int outerHeight = 0;
        outerSize(nullptr, width, height, state.style, state.exStyle, outerWidth, outerHeight);

        int origin = style == NativeWindowStyle::Borderless ? 0 : CW_USEDEFAULT;
        HWND created = CreateWindowExW(
            state.exStyle,
            kClassName,
            titleWide(title).c_str(),
            state.style,
            origin,
            origin,
            outerWidth,
            outerHeight,
            nullptr,
            nullptr,
            instance,
            this);
        if (!created) {
            detachState(state);
            return false;
        }

        state.hwnd = (void*)created;
        if (style != NativeWindowStyle::Borderless) {
            useDarkCaption(created);
        }
        return true;
    }

    void NativeWindow::destroy()
    {
        HWND window = hwnd(*this);
        if (!window) {
            return;
        }

        if (onClose) {
            auto callback = std::move(onClose);
            callback();
        }

        platform(*this).pendingHigh = 0;
        detachState(platform(*this));
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        if (GetCapture() == window) {
            ReleaseCapture();
        }
        DestroyWindow(window);
    }

    void NativeWindow::show(bool activate)
    {
        HWND window = hwnd(*this);
        if (!window) {
            return;
        }
        ShowWindow(window, activate ? SW_SHOW : SW_SHOWNOACTIVATE);
        if (activate) {
            UpdateWindow(window);
        }
    }

    void NativeWindow::hide()
    {
        HWND window = hwnd(*this);
        if (window) {
            ShowWindow(window, SW_HIDE);
        }
    }

    void NativeWindow::close()
    {
        HWND window = hwnd(*this);
        if (window) {
            SendMessageW(window, WM_CLOSE, 0, 0);
        }
    }

    void NativeWindow::setTitle(const String& title)
    {
        HWND window = hwnd(*this);
        if (window) {
            SetWindowTextW(window, titleWide(title).c_str());
        }
    }

    String NativeWindow::title() const
    {
        HWND window = hwnd(*this);
        if (!window) {
            return String();
        }
        int length = GetWindowTextLengthW(window);
        if (length <= 0) {
            return String();
        }
        std::wstring text((size_t)length + 1, L'\0');
        GetWindowTextW(window, text.data(), length + 1);
        text.resize((size_t)length);
        return String::unicodeToUtf8(text, false);
    }

    void NativeWindow::setClientSize(float width, float height)
    {
        HWND window = hwnd(*this);
        if (!window) {
            return;
        }
        const NativeWindowState& state = platform(*this);
        int outerWidth = 0;
        int outerHeight = 0;
        outerSize(window, width, height, state.style, state.exStyle, outerWidth, outerHeight);
        SetWindowPos(window, nullptr, 0, 0, outerWidth, outerHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    Vector2 NativeWindow::clientSize() const
    {
        HWND window = hwnd(*this);
        if (!window) {
            return Vector2();
        }
        RECT rect = {};
        if (!GetClientRect(window, &rect)) {
            return Vector2();
        }
        return Vector2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
    }

    void NativeWindow::setPosition(float x, float y)
    {
        HWND window = hwnd(*this);
        if (window) {
            SetWindowPos(window, nullptr, (int)x, (int)y, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOCOPYBITS);
        }
    }

    Vector2 NativeWindow::position() const
    {
        HWND window = hwnd(*this);
        if (!window) {
            return Vector2();
        }
        RECT rect = {};
        if (!GetWindowRect(window, &rect)) {
            return Vector2();
        }
        return Vector2((float)rect.left, (float)rect.top);
    }

    void NativeWindow::center()
    {
        HWND window = hwnd(*this);
        if (!window) {
            return;
        }
        HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info = {};
        info.cbSize = sizeof(info);
        if (!GetMonitorInfoW(monitor, &info)) {
            return;
        }
        RECT rect = {};
        if (!GetWindowRect(window, &rect)) {
            return;
        }
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        int x = info.rcWork.left + ((info.rcWork.right - info.rcWork.left) - width) / 2;
        int y = info.rcWork.top + ((info.rcWork.bottom - info.rcWork.top) - height) / 2;
        SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void NativeWindow::focus()
    {
        HWND window = hwnd(*this);
        if (window) {
            SetForegroundWindow(window);
            SetFocus(window);
        }
    }

    void NativeWindow::minimize()
    {
        HWND window = hwnd(*this);
        if (window) {
            ShowWindow(window, SW_MINIMIZE);
        }
    }

    void NativeWindow::maximize()
    {
        HWND window = hwnd(*this);
        if (window) {
            ShowWindow(window, SW_MAXIMIZE);
        }
    }

    void NativeWindow::restore()
    {
        HWND window = hwnd(*this);
        if (window) {
            ShowWindow(window, SW_RESTORE);
        }
    }

    bool NativeWindow::isOpen() const
    {
        return hwnd(*this) != nullptr;
    }

    bool NativeWindow::isVisible() const
    {
        HWND window = hwnd(*this);
        return window && IsWindowVisible(window);
    }

    bool NativeWindow::isMinimized() const
    {
        HWND window = hwnd(*this);
        return window && IsIconic(window);
    }

    bool NativeWindow::isMaximized() const
    {
        HWND window = hwnd(*this);
        return window && IsZoomed(window);
    }

    void NativeWindow::pumpMessages()
    {
        HWND window = hwnd(*this);
        if (!window) {
            return;
        }
        MSG message = {};
        while (PeekMessageW(&message, window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
            if (hwnd(*this) != window) {
                break;
            }
        }
    }

    void* NativeWindow::handle() const
    {
        return platform(*this).hwnd;
    }

    void* NativeWindow::display() const
    {
        return nullptr;
    }
}
