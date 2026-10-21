#include "../NativeInput.h"

namespace eokas
{
    // platformKey is NSEvent.keyCode. Numbers are kVK_* from HIToolbox/Events.h.
    NativeKeyCode transferKeyCode(int platformKey, bool)
    {
        switch (platformKey) {
        case 0x33: return NativeKeyCode::Backspace; // kVK_Delete
        case 0x30: return NativeKeyCode::Tab; // kVK_Tab
        case 0x24: return NativeKeyCode::Enter; // kVK_Return
        case 0x35: return NativeKeyCode::Escape; // kVK_Escape
        case 0x31: return NativeKeyCode::Space; // kVK_Space
        case 0x72: return NativeKeyCode::Insert; // kVK_Help
        case 0x75: return NativeKeyCode::Delete; // kVK_ForwardDelete
        case 0x73: return NativeKeyCode::Home; // kVK_Home
        case 0x77: return NativeKeyCode::End; // kVK_End
        case 0x74: return NativeKeyCode::PageUp; // kVK_PageUp
        case 0x79: return NativeKeyCode::PageDown; // kVK_PageDown
        case 0x7B: return NativeKeyCode::Left; // kVK_LeftArrow
        case 0x7C: return NativeKeyCode::Right; // kVK_RightArrow
        case 0x7E: return NativeKeyCode::Up; // kVK_UpArrow
        case 0x7D: return NativeKeyCode::Down; // kVK_DownArrow
        case 0x00: return NativeKeyCode::A; // kVK_ANSI_A
        case 0x0B: return NativeKeyCode::B; // kVK_ANSI_B
        case 0x08: return NativeKeyCode::C; // kVK_ANSI_C
        case 0x02: return NativeKeyCode::D; // kVK_ANSI_D
        case 0x0E: return NativeKeyCode::E; // kVK_ANSI_E
        case 0x03: return NativeKeyCode::F; // kVK_ANSI_F
        case 0x05: return NativeKeyCode::G; // kVK_ANSI_G
        case 0x04: return NativeKeyCode::H; // kVK_ANSI_H
        case 0x22: return NativeKeyCode::I; // kVK_ANSI_I
        case 0x26: return NativeKeyCode::J; // kVK_ANSI_J
        case 0x28: return NativeKeyCode::K; // kVK_ANSI_K
        case 0x25: return NativeKeyCode::L; // kVK_ANSI_L
        case 0x2E: return NativeKeyCode::M; // kVK_ANSI_M
        case 0x2D: return NativeKeyCode::N; // kVK_ANSI_N
        case 0x1F: return NativeKeyCode::O; // kVK_ANSI_O
        case 0x23: return NativeKeyCode::P; // kVK_ANSI_P
        case 0x0C: return NativeKeyCode::Q; // kVK_ANSI_Q
        case 0x0F: return NativeKeyCode::R; // kVK_ANSI_R
        case 0x01: return NativeKeyCode::S; // kVK_ANSI_S
        case 0x11: return NativeKeyCode::T; // kVK_ANSI_T
        case 0x20: return NativeKeyCode::U; // kVK_ANSI_U
        case 0x09: return NativeKeyCode::V; // kVK_ANSI_V
        case 0x0D: return NativeKeyCode::W; // kVK_ANSI_W
        case 0x07: return NativeKeyCode::X; // kVK_ANSI_X
        case 0x10: return NativeKeyCode::Y; // kVK_ANSI_Y
        case 0x06: return NativeKeyCode::Z; // kVK_ANSI_Z
        case 0x1D: return NativeKeyCode::Digit0; // kVK_ANSI_0
        case 0x12: return NativeKeyCode::Digit1; // kVK_ANSI_1
        case 0x13: return NativeKeyCode::Digit2; // kVK_ANSI_2
        case 0x14: return NativeKeyCode::Digit3; // kVK_ANSI_3
        case 0x15: return NativeKeyCode::Digit4; // kVK_ANSI_4
        case 0x17: return NativeKeyCode::Digit5; // kVK_ANSI_5
        case 0x16: return NativeKeyCode::Digit6; // kVK_ANSI_6
        case 0x1A: return NativeKeyCode::Digit7; // kVK_ANSI_7
        case 0x1C: return NativeKeyCode::Digit8; // kVK_ANSI_8
        case 0x19: return NativeKeyCode::Digit9; // kVK_ANSI_9
        case 0x7A: return NativeKeyCode::F1; // kVK_F1
        case 0x78: return NativeKeyCode::F2; // kVK_F2
        case 0x63: return NativeKeyCode::F3; // kVK_F3
        case 0x76: return NativeKeyCode::F4; // kVK_F4
        case 0x60: return NativeKeyCode::F5; // kVK_F5
        case 0x61: return NativeKeyCode::F6; // kVK_F6
        case 0x62: return NativeKeyCode::F7; // kVK_F7
        case 0x64: return NativeKeyCode::F8; // kVK_F8
        case 0x65: return NativeKeyCode::F9; // kVK_F9
        case 0x6D: return NativeKeyCode::F10; // kVK_F10
        case 0x67: return NativeKeyCode::F11; // kVK_F11
        case 0x6F: return NativeKeyCode::F12; // kVK_F12
        case 0x27: return NativeKeyCode::Apostrophe; // kVK_ANSI_Quote
        case 0x2B: return NativeKeyCode::Comma; // kVK_ANSI_Comma
        case 0x1B: return NativeKeyCode::Minus; // kVK_ANSI_Minus
        case 0x2F: return NativeKeyCode::Period; // kVK_ANSI_Period
        case 0x2C: return NativeKeyCode::Slash; // kVK_ANSI_Slash
        case 0x29: return NativeKeyCode::Semicolon; // kVK_ANSI_Semicolon
        case 0x18: return NativeKeyCode::Equal; // kVK_ANSI_Equal
        case 0x21: return NativeKeyCode::LeftBracket; // kVK_ANSI_LeftBracket
        case 0x2A: return NativeKeyCode::Backslash; // kVK_ANSI_Backslash
        case 0x1E: return NativeKeyCode::RightBracket; // kVK_ANSI_RightBracket
        case 0x32: return NativeKeyCode::GraveAccent; // kVK_ANSI_Grave
        case 0x39: return NativeKeyCode::CapsLock; // kVK_CapsLock
        case 0x47: return NativeKeyCode::NumLock; // kVK_ANSI_KeypadClear
        case 0x52: return NativeKeyCode::Keypad0; // kVK_ANSI_Keypad0
        case 0x53: return NativeKeyCode::Keypad1; // kVK_ANSI_Keypad1
        case 0x54: return NativeKeyCode::Keypad2; // kVK_ANSI_Keypad2
        case 0x55: return NativeKeyCode::Keypad3; // kVK_ANSI_Keypad3
        case 0x56: return NativeKeyCode::Keypad4; // kVK_ANSI_Keypad4
        case 0x57: return NativeKeyCode::Keypad5; // kVK_ANSI_Keypad5
        case 0x58: return NativeKeyCode::Keypad6; // kVK_ANSI_Keypad6
        case 0x59: return NativeKeyCode::Keypad7; // kVK_ANSI_Keypad7
        case 0x5B: return NativeKeyCode::Keypad8; // kVK_ANSI_Keypad8
        case 0x5C: return NativeKeyCode::Keypad9; // kVK_ANSI_Keypad9
        case 0x41: return NativeKeyCode::KeypadDecimal; // kVK_ANSI_KeypadDecimal
        case 0x4B: return NativeKeyCode::KeypadDivide; // kVK_ANSI_KeypadDivide
        case 0x43: return NativeKeyCode::KeypadMultiply; // kVK_ANSI_KeypadMultiply
        case 0x4E: return NativeKeyCode::KeypadSubtract; // kVK_ANSI_KeypadMinus
        case 0x45: return NativeKeyCode::KeypadAdd; // kVK_ANSI_KeypadPlus
        case 0x4C: return NativeKeyCode::KeypadEnter; // kVK_ANSI_KeypadEnter
        case 0x51: return NativeKeyCode::KeypadEqual; // kVK_ANSI_KeypadEquals
        default: return NativeKeyCode::Unknown;
        }
    }

    // platformButton is NSEvent.buttonNumber.
    NativeMouseButton transferMouseButton(int platformButton)
    {
        switch (platformButton) {
        case 0: return NativeMouseButton::Left;
        case 1: return NativeMouseButton::Right;
        case 2: return NativeMouseButton::Middle;
        case 3: return NativeMouseButton::X1;
        case 4: return NativeMouseButton::X2;
        default: return NativeMouseButton::Unknown;
        }
    }
}
