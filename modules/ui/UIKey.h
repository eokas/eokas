#ifndef _EOKAS_UI_KEY_H_
#define _EOKAS_UI_KEY_H_

namespace eokas
{
    enum class UIKey
    {
        Backspace,
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

    struct UIKeyMods
    {
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
    };
}

#endif//_EOKAS_UI_KEY_H_
