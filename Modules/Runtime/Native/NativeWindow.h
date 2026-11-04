#ifndef _EOKAS_NATIVE_WINDOW_H_
#define _EOKAS_NATIVE_WINDOW_H_

#include "./Header.h"
#include "./NativeInput.h"

#include <functional>
#include <memory>

namespace eokas
{
    enum class NativeWindowStyle
    {
        Standard,
        Borderless,
    };

    enum class NativeCursor
    {
        Arrow,
        SizeWestEast,
        SizeNorthSouth,
    };

    struct NativeWindowState;

    class NativeWindow
    {
    public:
        std::unique_ptr<NativeWindowState> state;

        NativeWindow();
        ~NativeWindow();

        NativeWindow(const NativeWindow&) = delete;
        NativeWindow& operator=(const NativeWindow&) = delete;

        // Client size is the drawable area, in pixels.
        // Borderless windows have no title bar; the frame origin is the client origin.
        bool create(const String& title, float width, float height, NativeWindowStyle style = NativeWindowStyle::Standard);
        void destroy();

        void show(bool activate = true);
        void hide();
        void close();

        void setTitle(const String& title);
        String title() const;

        void setClientSize(float width, float height);
        Vector2 clientSize() const;

        // Frame origin on the screen. Top-left, y grows downward.
        void setPosition(float x, float y);
        Vector2 position() const;

        void center();
        void focus();

        void minimize();
        void maximize();
        void restore();

        bool isOpen() const;
        bool isVisible() const;
        bool isMinimized() const;
        bool isMaximized() const;

        void pumpMessages();

        // Platform window. HWND on Windows, NSWindow* on macOS, X11 Window on Linux.
        void* handle() const;

        // Display connection for handle(). X11 Display* on Linux. Null on Windows and macOS.
        void* display() const;

        // Fired when the user or the owner closes the window, before the platform window is destroyed.
        std::function<void()> onClose;
        std::function<void(float width, float height)> onClientSize;
        std::function<void(float x, float y)> onMouseMove;
        // button is NativeMouseButton::Unknown when the platform button is unknown.
        std::function<void(float x, float y, NativeMouseButton button)> onMouseDown;
        std::function<void(float x, float y, NativeMouseButton button)> onMouseUp;
        std::function<void(float x, float y, float deltaX, float deltaY)> onMouseWheel;
        // Return true when the key is consumed.
        // keyCode is NativeKeyCode::Unknown when the platform key is unknown.
        std::function<bool(NativeKeyCode keyCode, bool extended, bool ctrl, bool shift, bool alt, bool repeat)> onKeyPress;
        std::function<bool(NativeKeyCode keyCode, bool extended, bool ctrl, bool shift, bool alt)> onKeyRelease;
        // Return true when the character is consumed.
        std::function<bool(uint32_t character)> onKeyChar;
        // Client-area cursor. Return true to apply `cursor`.
        std::function<bool(float x, float y, NativeCursor& cursor)> onCursor;
    };
}

#endif//_EOKAS_NATIVE_WINDOW_H_
