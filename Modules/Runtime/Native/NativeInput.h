#ifndef _EOKAS_NATIVE_INPUT_H_
#define _EOKAS_NATIVE_INPUT_H_

#include "./Header.h"

namespace eokas
{
    // Generic key codes shared by every platform.
    // NativeWindow::onKeyPress receives these values.
    // 0 is unused. Unknown is returned when a platform key has no generic code.
    enum class NativeKeyCode : int
    {
        Unknown = -1,

        Backspace = 1,
        Tab,
        Enter,
        Escape,
        Space,

        Insert,
        Delete,
        Home,
        End,
        PageUp,
        PageDown,
        Left,
        Right,
        Up,
        Down,

        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        Digit0, Digit1, Digit2, Digit3, Digit4,
        Digit5, Digit6, Digit7, Digit8, Digit9,

        F1, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,

        Apostrophe,
        Comma,
        Minus,
        Period,
        Slash,
        Semicolon,
        Equal,
        LeftBracket,
        Backslash,
        RightBracket,
        GraveAccent,

        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,

        Keypad0, Keypad1, Keypad2, Keypad3, Keypad4,
        Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
        KeypadDecimal,
        KeypadDivide,
        KeypadMultiply,
        KeypadSubtract,
        KeypadAdd,
        KeypadEnter,
        KeypadEqual,

        Menu,
    };

    // Generic mouse buttons shared by every platform.
    // NativeWindow::onMouseDown and onMouseUp receive these values.
    // Unknown is returned when a platform button has no generic button.
    enum class NativeMouseButton : int
    {
        Unknown = -1,
        Left = 0,
        Right,
        Middle,
        X1,
        X2,
    };

    // platformKey: Win32 virtual-key, macOS NSEvent.keyCode, or X11 KeySym.
    // extended: Win32 WM_KEYDOWN extended bit. Unused on macOS and Linux.
    NativeKeyCode transferKeyCode(int platformKey, bool extended);

    // platformButton: Win32 VK_LBUTTON / VK_RBUTTON / VK_MBUTTON / VK_XBUTTON1 / VK_XBUTTON2,
    // macOS NSEvent.buttonNumber, or an X11 button number.
    NativeMouseButton transferMouseButton(int platformButton);
}

#endif//_EOKAS_NATIVE_INPUT_H_
