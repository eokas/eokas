#ifndef _EOKAS_NATIVE_WINDOW_H_
#define _EOKAS_NATIVE_WINDOW_H_

#include "./Header.h"

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
        std::function<void(float x, float y, int button)> onMouseDown;
        std::function<void(float x, float y, int button)> onMouseUp;
        std::function<void(float x, float y, float deltaX, float deltaY)> onMouseWheel;
        // Return true when the key is consumed.
        // keyCode is the platform virtual-key code: Win32 VK_*, macOS keyCode, X11 KeySym.
        std::function<bool(int keyCode, bool extended, bool ctrl, bool shift, bool alt)> onKeyDown;
        // Return true when the character is consumed.
        std::function<bool(uint32_t codepoint)> onChar;
        // Client-area cursor. Return true to apply `cursor`.
        std::function<bool(float x, float y, NativeCursor& cursor)> onCursor;
    };
}

#endif//_EOKAS_NATIVE_WINDOW_H_
