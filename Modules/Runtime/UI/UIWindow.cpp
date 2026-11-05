#include "UIWindow.h"

#include <chrono>
#include <cstddef>

namespace eokas
{
    namespace
    {
        constexpr const char* kUIShaderPath = "Content/Shaders/UI.hlsl";

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

        UIPointerEvent makePointerEvent(float x, float y, UIPointerCode code, const Vector2& delta = Vector2::ZERO)
        {
            UIPointerEvent event;
            event.pointerCode = code;
            event.position = Vector2(x, y);
            event.deltaPosition = delta;
            return event;
        }

        void notifyPointerPressAnywhere(UIWidget* widget, const UIPointerEvent& event)
        {
            if (widget == nullptr || !widget->visible)
            {
                return;
            }
            widget->handlePointerPressAnywhere(event);
            for (const auto& child : widget->children())
            {
                notifyPointerPressAnywhere(child.get(), event);
            }
        }

        void notifyPointerReleaseAnywhere(UIWidget* widget, const UIPointerEvent& event)
        {
            if (widget == nullptr || !widget->visible)
            {
                return;
            }
            widget->handlePointerReleaseAnywhere(event);
            for (const auto& child : widget->children())
            {
                notifyPointerReleaseAnywhere(child.get(), event);
            }
        }
    }

    void UIWindow::init(uint32_t width, uint32_t height)
    {
        mWidth = (float)width;
        mHeight = (float)height;
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = UIPointerCode::Unknown;
        mPointerX = 0.0f;
        mPointerY = 0.0f;
        mHasPointer = false;
        mRawInputEvents.clear();
        mLastParentPivot = Vector2(0.0f, 0.0f);
        mLastWidget = nullptr;
        mHasParentPivot = false;
        mFocused = nullptr;

        if (!mRoot)
        {
            mRoot = std::make_shared<UIWidget>();
            mRoot->shape.pivot = Vector2(0.0f, 0.0f);
            mRoot->shape.setBox(Vector2(0.0f, 0.0f), Vector2(mWidth, mHeight));
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
        mPressedButton = UIPointerCode::Unknown;
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

    const UIWidget::Ref& UIWindow::root() const
    {
        return mRoot;
    }

    void UIWindow::setRoot(const UIWidget::Ref& widget)
    {
        this->setFocus(nullptr);
        this->resetPointerState(mRoot.get());
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = UIPointerCode::Unknown;
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
        this->refreshPointerTargets();
        if (mHasPointer && mPressed == nullptr)
        {
            this->updateHover(mPointerX, mPointerY);
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

    void UIWindow::updateHover(float x, float y)
    {
        UIWidget* hit = this->hitTest(x, y);
        if (hit == mHovered)
        {
            return;
        }
        if (mHovered != nullptr)
        {
            mHovered->handlePointerLeave(makePointerEvent(x, y, UIPointerCode::Unknown));
        }
        mHovered = hit;
        if (mHovered != nullptr)
        {
            mHovered->handlePointerEnter(makePointerEvent(x, y, UIPointerCode::Unknown));
        }
    }

    void UIWindow::onMouseMove(float x, float y)
    {
        mPointerX = x;
        mPointerY = y;
        mHasPointer = true;
        this->refreshPointerTargets();
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
            return;
        }
        this->updateHover(x, y);
        this->dispatchPointerMove(mHovered, x, y);
    }

    void UIWindow::onMouseDown(float x, float y, UIPointerCode button)
    {
        this->dispatchPointerPressAnywhere(x, y, button);
        this->onMouseMove(x, y);
        if (mPressed != nullptr || mHovered == nullptr)
        {
            if (mPressed == nullptr && button == UIPointerCode::Left)
            {
                this->setFocus(nullptr);
            }
            return;
        }
        mPressed = mHovered;
        mPressedButton = button;
        if (button == UIPointerCode::Left)
        {
            bool insideFocus = mFocused != nullptr && mFocused->containsDescendant(mPressed);
            if (mPressed->focusable)
            {
                this->setFocus(mPressed);
            }
            else if (!insideFocus)
            {
                this->setFocus(nullptr);
            }
        }
        mRawInputEvents.clear();
        mPressed->handlePointerPress(makePointerEvent(x, y, button));
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

    void UIWindow::onMouseUp(float x, float y, UIPointerCode button)
    {
        this->dispatchPointerReleaseAnywhere(x, y, button);
        this->refreshPointerTargets();
        UIWidget* hit = this->hitTest(x, y);
        UIWidget* pressed = mPressed;
        const UIPointerCode pressedButton = mPressedButton;
        mPressed = nullptr;
        mPressedButton = UIPointerCode::Unknown;
        if (pressed != nullptr && button == pressedButton)
        {
            endActiveDrags(mRoot.get());
            pressed->handlePointerRelease(makePointerEvent(x, y, button));
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
        this->refreshPointerTargets();
        UIInputInfo wheel;
        wheel.target = this->hitTest(x, y);
        wheel.type = UIInputEvent::Wheel;
        wheel.x = x;
        wheel.y = y;
        wheel.time = std::chrono::steady_clock::now();
        wheel.wheelDeltaX = deltaX;
        wheel.wheelDeltaY = deltaY;
        this->appendRawInput(wheel);
        UIWidget* walker = this->hitTest(x, y);
        while (walker != nullptr)
        {
            if (walker->handleWheel(x, y, deltaX, deltaY))
            {
                break;
            }
            walker = walker->parent();
        }
        this->clearRawInputEvents();
    }

    void UIWindow::dispatchPointerPressAnywhere(float x, float y, UIPointerCode button)
    {
        notifyPointerPressAnywhere(mRoot.get(), makePointerEvent(x, y, button));
    }

    void UIWindow::dispatchPointerReleaseAnywhere(float x, float y, UIPointerCode button)
    {
        notifyPointerReleaseAnywhere(mRoot.get(), makePointerEvent(x, y, button));
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
        if (mPressedButton != UIPointerCode::Left)
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
        Vector2 parentPivot = widget->screenToParentPivot(Vector2(x, y));
        Vector2 delta = (mHasParentPivot && mLastWidget == widget) ? (parentPivot - mLastParentPivot) : Vector2::ZERO;
        mLastParentPivot = parentPivot;
        mLastWidget = widget;
        mHasParentPivot = true;
        widget->handlePointerMove(makePointerEvent(x, y, mPressedButton, delta));
    }

    void UIWindow::onKeyChar(uint32_t character)
    {
        if (!this->focusAlive())
        {
            return;
        }
        mFocused->handleKeyChar(character);
    }

    void UIWindow::onKeyPress(const UIKeyEvent& event)
    {
        if (!this->focusAlive())
        {
            return;
        }
        if (mFocused->handleKeyPress(event))
        {
            return;
        }
        if (event.key == UIKeyCode::Escape)
        {
            this->setFocus(nullptr);
        }
    }

    void UIWindow::onKeyRelease(const UIKeyEvent& event)
    {
        if (!this->focusAlive())
        {
            return;
        }
        mFocused->handleKeyRelease(event);
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

    UIWidget* UIWindow::pressed() const
    {
        return mPressed;
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

    bool UIWindow::widgetAlive(const UIWidget* widget) const
    {
        return widget != nullptr && mRoot && mRoot->containsDescendant(widget);
    }

    void UIWindow::refreshPointerTargets()
    {
        if (mHovered != nullptr && !this->widgetAlive(mHovered))
        {
            mHovered = nullptr;
        }
        this->focusAlive();
    }

    void UIWindow::appendRawInput(const UIInputInfo& info)
    {
        mRawInputEvents.push_back(info);
    }

    void UIWindow::clearRawInputEvents()
    {
        mRawInputEvents.clear();
    }

    void UIWindow::dispatchPointerGesture(UIWidget* pressed, UIWidget* hit, UIPointerCode button, float x, float y)
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

    bool UIWindow::isClickGesture(const std::vector<UIInputInfo>& events, UIWidget* pressed, UIWidget* hit, UIPointerCode button) const
    {
        if (this->pointerDragged(events) || hit != pressed || button != UIPointerCode::Left)
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
