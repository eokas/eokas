#include "UIWindow.h"

#include <chrono>
#include <cstddef>

namespace eokas
{
    namespace
    {
        constexpr const char* kUIShaderPath = "shaders/UI.hlsl";

        Vector2 screenToParentPivot(const UIWidget& widget, const Vector2& screen)
        {
            if (widget.parent() == nullptr)
            {
                return screen;
            }
            return widget.parent()->screenToPivot(screen);
        }

        bool keyFromCodepoint(uint32_t codepoint, UIKey& key, UIKeyMods& mods)
        {
            mods = UIKeyMods();
            if (codepoint >= 'a' && codepoint <= 'z')
            {
                key = static_cast<UIKey>(static_cast<int>(UIKey::A) + static_cast<int>(codepoint - 'a'));
                return true;
            }
            if (codepoint >= 'A' && codepoint <= 'Z')
            {
                key = static_cast<UIKey>(static_cast<int>(UIKey::A) + static_cast<int>(codepoint - 'A'));
                mods.shift = true;
                return true;
            }
            if (codepoint >= '0' && codepoint <= '9')
            {
                key = static_cast<UIKey>(static_cast<int>(UIKey::Digit0) + static_cast<int>(codepoint - '0'));
                return true;
            }
            if (codepoint == ' ')
            {
                key = UIKey::Space;
                return true;
            }
            struct Punct
            {
                uint32_t plain;
                uint32_t shifted;
                UIKey key;
            };
            static const Punct punct[] = {
                {'\'', '"', UIKey::Apostrophe},
                {',', '<', UIKey::Comma},
                {'-', '_', UIKey::Minus},
                {'.', '>', UIKey::Period},
                {'/', '?', UIKey::Slash},
                {';', ':', UIKey::Semicolon},
                {'=', '+', UIKey::Equal},
                {'[', '{', UIKey::LeftBracket},
                {'\\', '|', UIKey::Backslash},
                {']', '}', UIKey::RightBracket},
                {'`', '~', UIKey::GraveAccent},
            };
            for (const Punct& item : punct)
            {
                if (codepoint == item.plain)
                {
                    key = item.key;
                    return true;
                }
                if (codepoint == item.shifted)
                {
                    key = item.key;
                    mods.shift = true;
                    return true;
                }
            }
            struct Digit
            {
                uint32_t shifted;
                UIKey key;
            };
            static const Digit digits[] = {
                {')', UIKey::Digit0},
                {'!', UIKey::Digit1},
                {'@', UIKey::Digit2},
                {'#', UIKey::Digit3},
                {'$', UIKey::Digit4},
                {'%', UIKey::Digit5},
                {'^', UIKey::Digit6},
                {'&', UIKey::Digit7},
                {'*', UIKey::Digit8},
                {'(', UIKey::Digit9},
            };
            for (const Digit& item : digits)
            {
                if (codepoint == item.shifted)
                {
                    key = item.key;
                    mods.shift = true;
                    return true;
                }
            }
            return false;
        }

        void endActiveDrags(UIWidget* widget)
        {
            if (widget == nullptr)
            {
                return;
            }
            widget->endActiveDrag();
            for (auto& child : widget->children())
            {
                if (child)
                {
                    endActiveDrags(child.get());
                }
            }
        }
    }

    void UIWindow::init(uint32_t width, uint32_t height)
    {
        mWidth = (float)width;
        mHeight = (float)height;
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mRawInputEvents.clear();
        mLastParentPivot = Vector2(0.0f, 0.0f);
        mLastWidget = nullptr;
        mHasParentPivot = false;
        mFocused = nullptr;

        if (!mRoot)
        {
            mRoot = std::make_shared<UIWidget>();
        }

        mPrimitive = std::make_shared<UIPrimitive>();
        mPrimitive->material = std::make_shared<Material>();
        mPrimitive->material->setShaderPath(kUIShaderPath);
        mPrimitive->material->addVertexElement({"POSITION", 0, (uint32_t)offsetof(UIVertex, position), Format::R32G32_FLOAT});
        mPrimitive->material->addVertexElement({"TEXCOORD", 0, (uint32_t)offsetof(UIVertex, uv), Format::R32G32_FLOAT});
        mPrimitive->material->addVertexElement({"COLOR", 0, (uint32_t)offsetof(UIVertex, color), Format::R32G32B32A32_FLOAT});
        mPrimitive->material->setCullMode(CullMode::None);
        DepthStencilState depthStencil;
        depthStencil.depthTest = false;
        depthStencil.depthWrite = false;
        depthStencil.depthFunc = CompareOp::Always;
        mPrimitive->material->setDepthStencilState(depthStencil);
        SamplerState sampler;
        sampler.minFilter = SamplerFilterMode::Linear;
        sampler.magFilter = SamplerFilterMode::Linear;
        sampler.mipFilter = SamplerFilterMode::Point;
        mPrimitive->material->setSamplerState(0, sampler);
        BlendState blend;
        blend.enabled = true;
        blend.srcColor = BlendFactor::SrcAlpha;
        blend.dstColor = BlendFactor::OneMinusSrcAlpha;
        blend.srcAlpha = BlendFactor::One;
        blend.dstAlpha = BlendFactor::OneMinusSrcAlpha;
        mPrimitive->material->setBlendState(blend);
    }

    void UIWindow::quit()
    {
        this->setFocus(nullptr);
        this->resetPointerState(mRoot.get());
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mRawInputEvents.clear();
        mLastParentPivot = Vector2(0.0f, 0.0f);
        mLastWidget = nullptr;
        mHasParentPivot = false;
        mRoot.reset();
        if (mPrimitive)
        {
            mPrimitive->material.reset();
        }
        mPrimitive.reset();
    }

    const std::shared_ptr<UIWidget>& UIWindow::root() const
    {
        return mRoot;
    }

    void UIWindow::setRoot(const std::shared_ptr<UIWidget>& widget)
    {
        this->setFocus(nullptr);
        this->resetPointerState(mRoot.get());
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mRawInputEvents.clear();
        mLastParentPivot = Vector2(0.0f, 0.0f);
        mLastWidget = nullptr;
        mHasParentPivot = false;
        if (widget && widget->parent() != nullptr)
        {
            widget->parent()->detachChild(widget.get());
        }
        mRoot = widget;
    }

    void UIWindow::tick(f32_t deltaTime)
    {
        (void)deltaTime;
        if (!mPrimitive)
        {
            return;
        }

        if (mRoot)
        {
            mRoot->layout();
        }
        mPrimitive->begin();
        if (mRoot)
        {
            mRoot->render(*mPrimitive);
        }
        mPrimitive->end();
    }

    UIPrimitive::Ref UIWindow::primitive() const
    {
        return mPrimitive;
    }

    UIWidget* UIWindow::hitTest(float x, float y)
    {
        if (!mRoot)
        {
            return nullptr;
        }
        return mRoot->pick(Vector2(x, y));
    }

    void UIWindow::onMouseMove(float x, float y)
    {
        if (mPressed != nullptr)
        {
            UIInputInfo move;
            move.target = mPressed;
            move.type = UIInputEvent::PointerMove;
            move.x = x;
            move.y = y;
            move.time = std::chrono::steady_clock::now();
            move.button = mPressedButton;
            this->appendRawInput(move);
            this->dispatchDrag(x, y);
        }

        UIWidget* hit = this->hitTest(x, y);
        if (hit != mHovered)
        {
            if (mHovered != nullptr)
            {
                mHovered->handlePointerLeave(x, y);
            }
            mHovered = hit;
            if (mHovered != nullptr)
            {
                mHovered->handlePointerEnter(x, y);
            }
        }
        if (mPressed == nullptr)
        {
            this->dispatchPointerMove(mHovered, x, y);
        }
    }

    void UIWindow::onMouseDown(float x, float y, int button)
    {
        this->onMouseMove(x, y);
        if (mPressed != nullptr || mHovered == nullptr)
        {
            if (mPressed == nullptr && button == 0)
            {
                this->setFocus(nullptr);
            }
            return;
        }
        mPressed = mHovered;
        mPressedButton = button;
        if (button == 0)
        {
            bool insideFocus = mFocused != nullptr && mFocused->containsDescendant(mPressed);
            if (mPressed->onKeyPress != nullptr || mPressed->onGotFocus != nullptr)
            {
                this->setFocus(mPressed);
            }
            else if (!insideFocus)
            {
                this->setFocus(nullptr);
            }
        }
        mRawInputEvents.clear();
        mPressed->handlePointerPress(x, y, button);
        UIInputInfo press;
        press.target = mPressed;
        press.type = UIInputEvent::PointerPress;
        press.x = x;
        press.y = y;
        press.time = std::chrono::steady_clock::now();
        press.button = button;
        this->appendRawInput(press);
        this->dispatchDrag(x, y);
    }

    void UIWindow::onMouseUp(float x, float y, int button)
    {
        UIWidget* hit = this->hitTest(x, y);
        UIWidget* pressed = mPressed;
        const int pressedButton = mPressedButton;
        mPressed = nullptr;
        mPressedButton = -1;
        if (pressed != nullptr && button == pressedButton)
        {
            endActiveDrags(mRoot.get());
            pressed->handlePointerRelease(x, y, button);
            UIInputInfo release;
            release.target = pressed;
            release.type = UIInputEvent::PointerRelease;
            release.x = x;
            release.y = y;
            release.time = std::chrono::steady_clock::now();
            release.button = button;
            this->appendRawInput(release);
            this->dispatchPointerGesture(pressed, hit, button, x, y);
        }
        else
        {
            this->clearRawInputEvents();
        }
        this->onMouseMove(x, y);
    }

    void UIWindow::onMouseWheel(float x, float y, float deltaX, float deltaY)
    {
        UIInputInfo wheel;
        wheel.target = this->hitTest(x, y);
        wheel.type = UIInputEvent::Wheel;
        wheel.x = x;
        wheel.y = y;
        wheel.time = std::chrono::steady_clock::now();
        wheel.wheelDeltaX = deltaX;
        wheel.wheelDeltaY = deltaY;
        this->appendRawInput(wheel);
        if (mRoot)
        {
            mRoot->handleWheel(x, y, deltaX, deltaY);
        }
        this->clearRawInputEvents();
    }

    void UIWindow::dispatchDrag(float x, float y)
    {
        if (mPressed == nullptr)
        {
            return;
        }
        Vector2 delta = this->dropDelta(mRawInputEvents);
        UIWidget* walker = mPressed;
        while (walker != nullptr)
        {
            if (walker->handleDrag(x, y, delta))
            {
                return;
            }
            walker = walker->parent();
        }
        if (mPressedButton != 0)
        {
            return;
        }
        this->dispatchPointerMove(mPressed, x, y);
    }

    void UIWindow::dispatchPointerMove(UIWidget* widget, float x, float y)
    {
        if (widget == nullptr)
        {
            return;
        }
        Vector2 parentPivot = screenToParentPivot(*widget, Vector2(x, y));
        Vector2 delta = (mHasParentPivot && mLastWidget == widget) ? (parentPivot - mLastParentPivot) : Vector2::ZERO;
        mLastParentPivot = parentPivot;
        mLastWidget = widget;
        mHasParentPivot = true;
        widget->handlePointerMove(x, y, delta);
    }

    void UIWindow::onChar(uint32_t codepoint)
    {
        if (!this->focusAlive())
        {
            return;
        }
        UIKey key = UIKey::Space;
        UIKeyMods mods;
        if (!keyFromCodepoint(codepoint, key, mods))
        {
            return;
        }
        mFocused->handleKeyPress(key, mods);
    }

    void UIWindow::onKeyDown(UIKey key, const UIKeyMods& mods)
    {
        if (key == UIKey::Escape)
        {
            this->setFocus(nullptr);
            return;
        }
        if (!this->focusAlive())
        {
            return;
        }
        mFocused->handleKeyPress(key, mods);
    }

    void UIWindow::setFocus(UIWidget* widget)
    {
        if (mFocused == widget)
        {
            return;
        }
        if (mFocused != nullptr)
        {
            mFocused->handleFocusLoss();
        }
        mFocused = widget;
        if (mFocused != nullptr)
        {
            mFocused->handleFocusGain();
        }
    }

    UIWidget* UIWindow::focus() const
    {
        return mFocused;
    }

    void UIWindow::resetPointerState(UIWidget* widget)
    {
        if (widget == nullptr)
        {
            return;
        }
        widget->resetPointerStateRecursive();
    }

    bool UIWindow::focusAlive()
    {
        if (mFocused == nullptr)
        {
            return false;
        }
        if (mRoot && mRoot->containsDescendant(mFocused))
        {
            return true;
        }
        mFocused->handleFocusLoss();
        mFocused = nullptr;
        return false;
    }

    void UIWindow::appendRawInput(const UIInputInfo& info)
    {
        mRawInputEvents.push_back(info);
    }

    void UIWindow::clearRawInputEvents()
    {
        mRawInputEvents.clear();
    }

    void UIWindow::dispatchPointerGesture(UIWidget* pressed, UIWidget* hit, int button, float x, float y)
    {
        const Vector2 delta = this->dropDelta(mRawInputEvents);
        if (this->isClickGesture(mRawInputEvents, pressed, hit, button))
        {
            pressed->handleClick(x, y);
        }
        else if (this->isDragGesture(mRawInputEvents))
        {
            UIWidget* walker = pressed;
            while (walker != nullptr)
            {
                if (walker->handleDrop(x, y, hit, delta))
                {
                    break;
                }
                walker = walker->parent();
            }
        }
        this->clearRawInputEvents();
    }

    const UIInputInfo* UIWindow::findPointerPress(const std::vector<UIInputInfo>& events) const
    {
        for (const UIInputInfo& event : events)
        {
            if (event.type == UIInputEvent::PointerPress)
            {
                return &event;
            }
        }
        return nullptr;
    }

    bool UIWindow::pointerPressOrigin(const std::vector<UIInputInfo>& events, float& x, float& y) const
    {
        const UIInputInfo* press = this->findPointerPress(events);
        if (press == nullptr)
        {
            return false;
        }
        x = press->x;
        y = press->y;
        return true;
    }

    bool UIWindow::pointerDragged(const std::vector<UIInputInfo>& events) const
    {
        const UIInputInfo* press = this->findPointerPress(events);
        if (press == nullptr)
        {
            return false;
        }
        for (const UIInputInfo& event : events)
        {
            if (event.type != UIInputEvent::PointerMove && event.type != UIInputEvent::PointerRelease)
            {
                continue;
            }
            float dx = event.x - press->x;
            float dy = event.y - press->y;
            if (dx * dx + dy * dy > 16.0f)
            {
                return true;
            }
        }
        return false;
    }

    bool UIWindow::isClickGesture(const std::vector<UIInputInfo>& events, UIWidget* pressed, UIWidget* hit, int button) const
    {
        if (this->pointerDragged(events) || hit != pressed || button != 0)
        {
            return false;
        }
        const UIInputInfo* press = this->findPointerPress(events);
        if (press == nullptr)
        {
            return false;
        }
        return press->target == pressed && press->button == button;
    }

    bool UIWindow::isDragGesture(const std::vector<UIInputInfo>& events) const
    {
        return this->pointerDragged(events) && this->findPointerPress(events) != nullptr;
    }

    Vector2 UIWindow::dropDelta(const std::vector<UIInputInfo>& events) const
    {
        float pressX = 0.0f;
        float pressY = 0.0f;
        if (!this->pointerPressOrigin(events, pressX, pressY) || events.empty())
        {
            return Vector2(0.0f, 0.0f);
        }
        const UIInputInfo& end = events.back();
        return Vector2(end.x - pressX, end.y - pressY);
    }
}
