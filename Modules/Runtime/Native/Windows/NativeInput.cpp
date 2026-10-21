#include "../NativeInput.h"

#include <windows.h>

namespace eokas
{
    NativeKeyCode transferKeyCode(int platformKey, bool extended)
    {
        switch (platformKey) {
        case VK_BACK: return NativeKeyCode::Backspace;
        case VK_TAB: return NativeKeyCode::Tab;
        case VK_RETURN: return extended ? NativeKeyCode::KeypadEnter : NativeKeyCode::Enter;
        case VK_ESCAPE: return NativeKeyCode::Escape;
        case VK_SPACE: return NativeKeyCode::Space;
        case VK_INSERT: return NativeKeyCode::Insert;
        case VK_DELETE: return NativeKeyCode::Delete;
        case VK_HOME: return NativeKeyCode::Home;
        case VK_END: return NativeKeyCode::End;
        case VK_PRIOR: return NativeKeyCode::PageUp;
        case VK_NEXT: return NativeKeyCode::PageDown;
        case VK_LEFT: return NativeKeyCode::Left;
        case VK_RIGHT: return NativeKeyCode::Right;
        case VK_UP: return NativeKeyCode::Up;
        case VK_DOWN: return NativeKeyCode::Down;
        case VK_OEM_7: return NativeKeyCode::Apostrophe;
        case VK_OEM_COMMA: return NativeKeyCode::Comma;
        case VK_OEM_MINUS: return NativeKeyCode::Minus;
        case VK_OEM_PERIOD: return NativeKeyCode::Period;
        case VK_OEM_2: return NativeKeyCode::Slash;
        case VK_OEM_1: return NativeKeyCode::Semicolon;
        case VK_OEM_PLUS: return NativeKeyCode::Equal;
        case VK_OEM_4: return NativeKeyCode::LeftBracket;
        case VK_OEM_5: return NativeKeyCode::Backslash;
        case VK_OEM_6: return NativeKeyCode::RightBracket;
        case VK_OEM_3: return NativeKeyCode::GraveAccent;
        case VK_CAPITAL: return NativeKeyCode::CapsLock;
        case VK_SCROLL: return NativeKeyCode::ScrollLock;
        case VK_NUMLOCK: return NativeKeyCode::NumLock;
        case VK_SNAPSHOT: return NativeKeyCode::PrintScreen;
        case VK_PAUSE: return NativeKeyCode::Pause;
        case VK_DECIMAL: return NativeKeyCode::KeypadDecimal;
        case VK_DIVIDE: return NativeKeyCode::KeypadDivide;
        case VK_MULTIPLY: return NativeKeyCode::KeypadMultiply;
        case VK_SUBTRACT: return NativeKeyCode::KeypadSubtract;
        case VK_ADD: return NativeKeyCode::KeypadAdd;
        case VK_OEM_NEC_EQUAL: return NativeKeyCode::KeypadEqual;
        case VK_APPS: return NativeKeyCode::Menu;
        default:
            break;
        }

        if (platformKey >= 'A' && platformKey <= 'Z') {
            return static_cast<NativeKeyCode>(static_cast<int>(NativeKeyCode::A) + (platformKey - 'A'));
        }
        if (platformKey >= '0' && platformKey <= '9') {
            return static_cast<NativeKeyCode>(static_cast<int>(NativeKeyCode::Digit0) + (platformKey - '0'));
        }
        if (platformKey >= VK_F1 && platformKey <= VK_F12) {
            return static_cast<NativeKeyCode>(static_cast<int>(NativeKeyCode::F1) + (platformKey - VK_F1));
        }
        if (platformKey >= VK_NUMPAD0 && platformKey <= VK_NUMPAD9) {
            return static_cast<NativeKeyCode>(static_cast<int>(NativeKeyCode::Keypad0) + (platformKey - VK_NUMPAD0));
        }
        return NativeKeyCode::Unknown;
    }

    NativeMouseButton transferMouseButton(int platformButton)
    {
        switch (platformButton) {
        case VK_LBUTTON: return NativeMouseButton::Left;
        case VK_RBUTTON: return NativeMouseButton::Right;
        case VK_MBUTTON: return NativeMouseButton::Middle;
        case VK_XBUTTON1: return NativeMouseButton::X1;
        case VK_XBUTTON2: return NativeMouseButton::X2;
        default: return NativeMouseButton::Unknown;
        }
    }
}
