#ifndef _EOKAS_UI_EVENT_H_
#define _EOKAS_UI_EVENT_H_

#include <chrono>

namespace eokas
{
    class UIWidget;

    // Generic key codes. Integer values match NativeKeyCode.
    // 0 is unused. Unknown is used when a key has no generic code.
    enum class UIKeyCode : int
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

    // Generic mouse buttons. Integer values match NativeMouseButton.
    // Unknown is used when a button has no generic button.
    enum class UIMouseButton : int
    {
        Unknown = -1,
        Left = 0,
        Right,
        Middle,
        X1,
        X2,
    };

    enum class UIInputEvent
    {
        None,
        PointerEnter,
        PointerLeave,
        PointerMove,
        PointerPress,
        PointerRelease,
        Wheel,
        KeyPress,
        Char,
    };

    struct UIInputInfo
    {
        UIWidget* target = nullptr;
        UIInputEvent type = UIInputEvent::None;
        float x = 0.0f;
        float y = 0.0f;
        std::chrono::steady_clock::time_point time {};
        UIMouseButton button = UIMouseButton::Unknown;
        float wheelDeltaX = 0.0f;
        float wheelDeltaY = 0.0f;
    };
}

#endif//_EOKAS_UI_EVENT_H_
