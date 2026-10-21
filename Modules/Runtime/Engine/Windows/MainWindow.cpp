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

namespace eokas {

    namespace {

        UIKeyCode transferKeyCode(NativeKeyCode keyCode) {
            return static_cast<UIKeyCode>(static_cast<int>(keyCode));
        }

        UIMouseButton transferMouseButton(NativeMouseButton mouseButton) {
            return static_cast<UIMouseButton>(static_cast<int>(mouseButton));
        }

        bool characterKey(UIKeyCode key) {
            int value = static_cast<int>(key);
            auto between = [value](UIKeyCode begin, UIKeyCode end) {
                return value >= static_cast<int>(begin) && value <= static_cast<int>(end);
            };
            if (between(UIKeyCode::A, UIKeyCode::Z)) return true;
            if (between(UIKeyCode::Digit0, UIKeyCode::Digit9)) return true;
            if (key == UIKeyCode::Space) return true;
            if (between(UIKeyCode::Apostrophe, UIKeyCode::GraveAccent)) return true;
            if (between(UIKeyCode::Keypad0, UIKeyCode::Keypad9)) return true;
            return key == UIKeyCode::KeypadDecimal
                || key == UIKeyCode::KeypadDivide
                || key == UIKeyCode::KeypadMultiply
                || key == UIKeyCode::KeypadSubtract
                || key == UIKeyCode::KeypadAdd
                || key == UIKeyCode::KeypadEqual;
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
        mNativeWindow.onMouseDown = [this](float x, float y, NativeMouseButton button) {
            if (mUiReady) {
                mUIWindow.onMouseDown(x, y, transferMouseButton(button));
            }
        };
        mNativeWindow.onMouseUp = [this](float x, float y, NativeMouseButton button) {
            if (mUiReady) {
                mUIWindow.onMouseUp(x, y, transferMouseButton(button));
            }
        };
        mNativeWindow.onMouseWheel = [this](float x, float y, float deltaX, float deltaY) {
            if (mUiReady) {
                mUIWindow.onMouseWheel(x, y, deltaX, deltaY);
            }
        };
        mNativeWindow.onKeyDown = [this](NativeKeyCode keyCode, bool, bool ctrl, bool shift, bool alt) {
            if (!mUiReady) {
                return false;
            }
            UIKeyCode key = transferKeyCode(keyCode);
            if (key == UIKeyCode::Unknown) {
                return false;
            }
            if (!characterKey(key) || ctrl || alt) {
                mUIWindow.onKeyDown(key, ctrl, shift, alt);
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
