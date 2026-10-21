#include "../NativeInput.h"

#include <X11/keysym.h>

namespace eokas
{
    NativeKeyCode transferKeyCode(int platformKey, bool)
    {
        switch (platformKey) {
        case XK_BackSpace: return NativeKeyCode::Backspace;
        case XK_Tab: return NativeKeyCode::Tab;
        case XK_Return: return NativeKeyCode::Enter;
        case XK_Escape: return NativeKeyCode::Escape;
        case XK_space: return NativeKeyCode::Space;
        case XK_Insert: return NativeKeyCode::Insert;
        case XK_Delete: return NativeKeyCode::Delete;
        case XK_Home: return NativeKeyCode::Home;
        case XK_End: return NativeKeyCode::End;
        case XK_Page_Up: return NativeKeyCode::PageUp;
        case XK_Page_Down: return NativeKeyCode::PageDown;
        case XK_Left: return NativeKeyCode::Left;
        case XK_Right: return NativeKeyCode::Right;
        case XK_Up: return NativeKeyCode::Up;
        case XK_Down: return NativeKeyCode::Down;
        case XK_apostrophe: return NativeKeyCode::Apostrophe;
        case XK_comma: return NativeKeyCode::Comma;
        case XK_minus: return NativeKeyCode::Minus;
        case XK_period: return NativeKeyCode::Period;
        case XK_slash: return NativeKeyCode::Slash;
        case XK_semicolon: return NativeKeyCode::Semicolon;
        case XK_equal: return NativeKeyCode::Equal;
        case XK_bracketleft: return NativeKeyCode::LeftBracket;
        case XK_backslash: return NativeKeyCode::Backslash;
        case XK_bracketright: return NativeKeyCode::RightBracket;
        case XK_grave: return NativeKeyCode::GraveAccent;
        case XK_Caps_Lock: return NativeKeyCode::CapsLock;
        case XK_Scroll_Lock: return NativeKeyCode::ScrollLock;
        case XK_Num_Lock: return NativeKeyCode::NumLock;
        case XK_Print: return NativeKeyCode::PrintScreen;
        case XK_Pause: return NativeKeyCode::Pause;
        case XK_KP_Decimal: return NativeKeyCode::KeypadDecimal;
        case XK_KP_Divide: return NativeKeyCode::KeypadDivide;
        case XK_KP_Multiply: return NativeKeyCode::KeypadMultiply;
        case XK_KP_Subtract: return NativeKeyCode::KeypadSubtract;
        case XK_KP_Add: return NativeKeyCode::KeypadAdd;
        case XK_KP_Enter: return NativeKeyCode::KeypadEnter;
        case XK_KP_Equal: return NativeKeyCode::KeypadEqual;
        case XK_Menu: return NativeKeyCode::Menu;
        default:
            break;
        }

        if (platformKey >= XK_a && platformKey <= XK_z) {
            return static_cast<NativeKeyCode>(static_cast<int>(NativeKeyCode::A) + (platformKey - XK_a));
        }
        if (platformKey >= XK_A && platformKey <= XK_Z) {
            return static_cast<NativeKeyCode>(static_cast<int>(NativeKeyCode::A) + (platformKey - XK_A));
        }
        if (platformKey >= XK_0 && platformKey <= XK_9) {
            return static_cast<NativeKeyCode>(static_cast<int>(NativeKeyCode::Digit0) + (platformKey - XK_0));
        }
        if (platformKey >= XK_F1 && platformKey <= XK_F12) {
            return static_cast<NativeKeyCode>(static_cast<int>(NativeKeyCode::F1) + (platformKey - XK_F1));
        }
        if (platformKey >= XK_KP_0 && platformKey <= XK_KP_9) {
            return static_cast<NativeKeyCode>(static_cast<int>(NativeKeyCode::Keypad0) + (platformKey - XK_KP_0));
        }
        return NativeKeyCode::Unknown;
    }

    NativeMouseButton transferMouseButton(int platformButton)
    {
        switch (platformButton) {
        case 1: return NativeMouseButton::Left;
        case 3: return NativeMouseButton::Right;
        case 2: return NativeMouseButton::Middle;
        case 8: return NativeMouseButton::X1;
        case 9: return NativeMouseButton::X2;
        default: return NativeMouseButton::Unknown;
        }
    }
}
