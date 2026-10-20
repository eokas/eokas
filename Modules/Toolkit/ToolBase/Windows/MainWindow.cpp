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
#include "../MainWindow.h"

#include <windows.h>

namespace eokas {

    namespace {

        bool mapUIKey(int code, bool extended, UIKey& key) {
            switch (code) {
            case VK_BACK: key = UIKey::Backspace; return true;
            case VK_TAB: key = UIKey::Tab; return true;
            case VK_RETURN: key = extended ? UIKey::KeypadEnter : UIKey::Enter; return true;
            case VK_ESCAPE: key = UIKey::Escape; return true;
            case VK_SPACE: key = UIKey::Space; return true;
            case VK_INSERT: key = UIKey::Insert; return true;
            case VK_DELETE: key = UIKey::Delete; return true;
            case VK_HOME: key = UIKey::Home; return true;
            case VK_END: key = UIKey::End; return true;
            case VK_PRIOR: key = UIKey::PageUp; return true;
            case VK_NEXT: key = UIKey::PageDown; return true;
            case VK_LEFT: key = UIKey::Left; return true;
            case VK_RIGHT: key = UIKey::Right; return true;
            case VK_UP: key = UIKey::Up; return true;
            case VK_DOWN: key = UIKey::Down; return true;
            case VK_OEM_7: key = UIKey::Apostrophe; return true;
            case VK_OEM_COMMA: key = UIKey::Comma; return true;
            case VK_OEM_MINUS: key = UIKey::Minus; return true;
            case VK_OEM_PERIOD: key = UIKey::Period; return true;
            case VK_OEM_2: key = UIKey::Slash; return true;
            case VK_OEM_1: key = UIKey::Semicolon; return true;
            case VK_OEM_PLUS: key = UIKey::Equal; return true;
            case VK_OEM_4: key = UIKey::LeftBracket; return true;
            case VK_OEM_5: key = UIKey::Backslash; return true;
            case VK_OEM_6: key = UIKey::RightBracket; return true;
            case VK_OEM_3: key = UIKey::GraveAccent; return true;
            case VK_CAPITAL: key = UIKey::CapsLock; return true;
            case VK_SCROLL: key = UIKey::ScrollLock; return true;
            case VK_NUMLOCK: key = UIKey::NumLock; return true;
            case VK_SNAPSHOT: key = UIKey::PrintScreen; return true;
            case VK_PAUSE: key = UIKey::Pause; return true;
            case VK_DECIMAL: key = UIKey::KeypadDecimal; return true;
            case VK_DIVIDE: key = UIKey::KeypadDivide; return true;
            case VK_MULTIPLY: key = UIKey::KeypadMultiply; return true;
            case VK_SUBTRACT: key = UIKey::KeypadSubtract; return true;
            case VK_ADD: key = UIKey::KeypadAdd; return true;
            case VK_OEM_NEC_EQUAL: key = UIKey::KeypadEqual; return true;
            case VK_APPS: key = UIKey::Menu; return true;
            default:
                break;
            }

            if (code >= 'A' && code <= 'Z') {
                key = static_cast<UIKey>(static_cast<int>(UIKey::A) + (code - 'A'));
                return true;
            }
            if (code >= '0' && code <= '9') {
                key = static_cast<UIKey>(static_cast<int>(UIKey::Digit0) + (code - '0'));
                return true;
            }
            if (code >= VK_F1 && code <= VK_F12) {
                key = static_cast<UIKey>(static_cast<int>(UIKey::F1) + (code - VK_F1));
                return true;
            }
            if (code >= VK_NUMPAD0 && code <= VK_NUMPAD9) {
                key = static_cast<UIKey>(static_cast<int>(UIKey::Keypad0) + (code - VK_NUMPAD0));
                return true;
            }
            return false;
        }

        bool characterKey(UIKey key) {
            int value = static_cast<int>(key);
            auto between = [value](UIKey begin, UIKey end) {
                return value >= static_cast<int>(begin) && value <= static_cast<int>(end);
            };
            if (between(UIKey::A, UIKey::Z)) return true;
            if (between(UIKey::Digit0, UIKey::Digit9)) return true;
            if (key == UIKey::Space) return true;
            if (between(UIKey::Apostrophe, UIKey::GraveAccent)) return true;
            if (between(UIKey::Keypad0, UIKey::Keypad9)) return true;
            return key == UIKey::KeypadDecimal
                || key == UIKey::KeypadDivide
                || key == UIKey::KeypadMultiply
                || key == UIKey::KeypadSubtract
                || key == UIKey::KeypadAdd
                || key == UIKey::KeypadEqual;
        }

    }

    void MainWindow::bindNativeInput() {
        mNativeWindow.onClose = [this]() {
            this->releaseSurface();
        };
        mNativeWindow.onClientSize = [this](float width, float height) {
            if (!mNativeWindow.isOpen()) {
                return;
            }
            this->applyClientSize(width, height);
        };
        mNativeWindow.onMouseMove = [this](float x, float y) {
            if (mUiReady) {
                mUIWindow.onMouseMove(x, y);
            }
        };
        mNativeWindow.onMouseDown = [this](float x, float y, int button) {
            if (mUiReady) {
                mUIWindow.onMouseDown(x, y, button);
            }
        };
        mNativeWindow.onMouseUp = [this](float x, float y, int button) {
            if (mUiReady) {
                mUIWindow.onMouseUp(x, y, button);
            }
        };
        mNativeWindow.onMouseWheel = [this](float x, float y, float deltaX, float deltaY) {
            if (mUiReady) {
                mUIWindow.onMouseWheel(x, y, deltaX, deltaY);
            }
        };
        mNativeWindow.onKeyDown = [this](int keyCode, bool extended, bool ctrl, bool shift, bool alt) {
            if (!mUiReady) {
                return false;
            }
            UIKey key = UIKey::Enter;
            if (!mapUIKey(keyCode, extended, key)) {
                return false;
            }
            UIKeyMods mods;
            mods.ctrl = ctrl;
            mods.shift = shift;
            mods.alt = alt;
            if (!characterKey(key) || mods.ctrl || mods.alt) {
                mUIWindow.onKeyDown(key, mods);
                return true;
            }
            return false;
        };
        mNativeWindow.onChar = [this](uint32_t codepoint) {
            if (!mUiReady) {
                return false;
            }
            if (codepoint >= 32 && codepoint != 127) {
                mUIWindow.onChar(codepoint);
                return true;
            }
            return false;
        };
    }

}
