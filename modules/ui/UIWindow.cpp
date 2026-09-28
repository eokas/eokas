#include "UIWindow.h"
#include "widgets/UICanvas.h"
#include "widgets/UIChart.h"
#include "widgets/UIView.h"

#include <cmath>
#include <cstddef>

namespace eokas
{
    namespace
    {
        constexpr const char* kUIShaderPath = "shaders/UI.hlsl";

        bool liveScale(const Vector2& scale)
        {
            return scale.x != 0.0f && scale.y != 0.0f;
        }

        Matrix3 parentSpaceOf(const UIWidget& widget)
        {
            if (widget.parent() == nullptr)
            {
                return Matrix3::IDENTITY;
            }
            return widget.parent()->pivotToScreen();
        }

        Vector2 screenToPivot(const UIWidget& widget, const Vector2& screen)
        {
            return UIShape::transformPoint(parentSpaceOf(widget).inverse(), screen);
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
    }

    void UIWindow::init(uint32_t width, uint32_t height)
    {
        mWidth = (float)width;
        mHeight = (float)height;
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mPressX = 0.0f;
        mPressY = 0.0f;
        mDragged = false;
        mLastLocal = Vector2(0.0f, 0.0f);
        mLastWidget = nullptr;
        mHasLocal = false;
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
        mPressX = 0.0f;
        mPressY = 0.0f;
        mDragged = false;
        mLastLocal = Vector2(0.0f, 0.0f);
        mLastWidget = nullptr;
        mHasLocal = false;
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
        mPressX = 0.0f;
        mPressY = 0.0f;
        mDragged = false;
        mLastLocal = Vector2(0.0f, 0.0f);
        mLastWidget = nullptr;
        mHasLocal = false;
        if (widget && widget->parent() != nullptr)
        {
            widget->parent()->detachChild(widget.get());
        }
        mRoot = widget;
    }

    void UIWindow::flush()
    {
        if (!mPrimitive)
        {
            return;
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
            float dx = x - mPressX;
            float dy = y - mPressY;
            if (dx * dx + dy * dy > 16.0f)
            {
                mDragged = true;
            }
            this->dispatchDrag(x, y);
        }

        UIWidget* hit = this->hitTest(x, y);
        if (hit != mHovered)
        {
            if (mHovered != nullptr)
            {
                mHovered->triggerPointerLeave();
            }
            mHovered = hit;
            if (mHovered != nullptr)
            {
                mHovered->triggerPointerEnter();
            }
        }
        if (mPressed == nullptr)
        {
            this->dispatchPointer(mHovered, x, y);
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
            bool insideFocus = mFocused != nullptr && this->containsWidget(mFocused, mPressed);
            if (mPressed->onKeyPress != nullptr || mPressed->onGotFocus != nullptr)
            {
                this->setFocus(mPressed);
            }
            else if (!insideFocus)
            {
                this->setFocus(nullptr);
            }
        }
        mPressX = x;
        mPressY = y;
        mDragged = false;
        mPressed->triggerPointerPress();
        this->dispatchDrag(x, y);
    }

    void UIWindow::onMouseUp(float x, float y, int button)
    {
        UIWidget* hit = this->hitTest(x, y);
        UIWidget* pressed = mPressed;
        const int pressedButton = mPressedButton;
        const bool dragged = mDragged;
        mPressed = nullptr;
        mPressedButton = -1;
        mDragged = false;
        if (pressed != nullptr && button == pressedButton)
        {
            this->endCanvasDrag(mRoot.get());
            const bool click = !dragged && hit == pressed && button == 0;
            UICanvas* canvas = nullptr;
            this->dragTargetOf(pressed, canvas);
            if (!click)
            {
                pressed->pressed = false;
            }
            pressed->triggerPointerRelease();
            if (click)
            {
                if (canvas != nullptr)
                {
                    if (UIChart* chart = dynamic_cast<UIChart*>(pressed))
                    {
                        canvas->select(chart);
                    }
                }
            }
            else if (dragged && canvas != nullptr)
            {
                if (UIChart* chart = dynamic_cast<UIChart*>(pressed))
                {
                    Vector2 drop = screenToPivot(*canvas, Vector2(x, y));
                    canvas->dispatchDrop(chart, hit, drop.x, drop.y);
                }
            }
        }
        this->onMouseMove(x, y);
    }

    void UIWindow::onMouseWheel(float x, float y, float deltaX, float deltaY)
    {
        this->routeWheel(mRoot.get(), Vector2(x, y), Vector2(deltaX, deltaY));
        if (UIWidget* hit = this->hitTest(x, y))
        {
            Vector2 local = screenToPivot(*hit, Vector2(x, y));
            hit->triggerWheel(local, deltaY);
        }
    }

    void UIWindow::dispatchDrag(float x, float y)
    {
        if (mPressed == nullptr)
        {
            return;
        }
        UICanvas* canvas = nullptr;
        UIWidget* target = this->dragTargetOf(mPressed, canvas);
        if (canvas != nullptr && target != nullptr && target != canvas)
        {
            Vector2 local = screenToPivot(*target, Vector2(x, y));
            canvas->dragChild(target, local.x, local.y);
            return;
        }
        if (mPressedButton != 0)
        {
            return;
        }
        this->dispatchPointer(mPressed, x, y);
    }

    void UIWindow::dispatchPointer(UIWidget* widget, float x, float y)
    {
        if (widget == nullptr)
        {
            return;
        }
        Vector2 local = screenToPivot(*widget, Vector2(x, y));
        Vector2 delta = (mHasLocal && mLastWidget == widget) ? (local - mLastLocal) : Vector2::ZERO;
        mLastLocal = local;
        mLastWidget = widget;
        mHasLocal = true;
        widget->triggerPointerMove(local, delta);
    }

    bool UIWindow::collectPath(UIWidget* node, UIWidget* target, std::vector<UIWidget*>& path) const
    {
        if (node == nullptr)
        {
            return false;
        }
        path.push_back(node);
        if (node == target)
        {
            return true;
        }
        if (UIView* view = dynamic_cast<UIView*>(node))
        {
            for (auto& child : view->root()->children())
            {
                if (this->collectPath(child.get(), target, path))
                {
                    return true;
                }
            }
            path.pop_back();
            return false;
        }
        for (auto& child : node->children())
        {
            if (this->collectPath(child.get(), target, path))
            {
                return true;
            }
        }
        path.pop_back();
        return false;
    }

    UIWidget* UIWindow::dragTargetOf(UIWidget* pressed, UICanvas*& canvas) const
    {
        canvas = nullptr;
        std::vector<UIWidget*> path;
        if (!this->collectPath(mRoot.get(), pressed, path))
        {
            return nullptr;
        }
        int canvasIndex = -1;
        for (int i = 0; i < (int)path.size(); ++i)
        {
            if (dynamic_cast<UICanvas*>(path[(size_t)i]) != nullptr)
            {
                canvasIndex = i;
            }
        }
        if (canvasIndex < 0)
        {
            return nullptr;
        }
        canvas = static_cast<UICanvas*>(path[(size_t)canvasIndex]);
        int viewIndex = -1;
        for (int i = canvasIndex + 1; i < (int)path.size(); ++i)
        {
            if (dynamic_cast<UIView*>(path[(size_t)i]) != nullptr)
            {
                viewIndex = i;
            }
        }
        int start = (int)path.size() - 1;
        if (viewIndex >= 0)
        {
            start = viewIndex;
        }
        for (int i = start; i > canvasIndex; --i)
        {
            if (path[(size_t)i]->dragable)
            {
                return path[(size_t)i];
            }
        }
        return nullptr;
    }

    bool UIWindow::routeWheel(UIWidget* widget, const Vector2& point, const Vector2& delta)
    {
        if (widget == nullptr || !widget->visible || !liveScale(widget->shape.scale))
        {
            return false;
        }
        if (UICanvas* canvas = dynamic_cast<UICanvas*>(widget))
        {
            for (auto it = canvas->children().rbegin(); it != canvas->children().rend(); ++it)
            {
                if (*it && this->routeNestedCanvas(it->get(), point, delta))
                {
                    return true;
                }
            }
            Vector2 parentLocal = screenToPivot(*canvas, point);
            Vector2 local = canvas->shape.toLocal(parentLocal);
            if (!Rect(0.0f, 0.0f, canvas->shape.size.x, canvas->shape.size.y).contains(local))
            {
                return false;
            }
            UIWidget* hit = canvas->pick(parentLocal);
            if (UIChart* chart = dynamic_cast<UIChart*>(hit))
            {
                if (liveScale(chart->shape.scale))
                {
                    float current = chart->shape.scale.x == 0.0f ? 1.0f : chart->shape.scale.x;
                    float factor = expf(-delta.y * chart->scaleSensitivity);
                    chart->scaleAt(screenToPivot(*chart, point), current * factor);
                    return true;
                }
            }
            float current = canvas->shape.scale.x;
            if (current == 0.0f)
            {
                current = 1.0f;
            }
            float factor = expf(-delta.y * canvas->scaleSensitivity);
            canvas->scaleAt(parentLocal, current * factor);
            return true;
        }
        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            const std::shared_ptr<UIWidget>& content = view->root();
            for (auto it = content->children().rbegin(); it != content->children().rend(); ++it)
            {
                if (*it && (*it)->floating && this->routeWheel(it->get(), point, delta))
                {
                    return true;
                }
            }
            for (auto it = content->children().rbegin(); it != content->children().rend(); ++it)
            {
                if (*it && !(*it)->floating && this->routeWheel(it->get(), point, delta))
                {
                    return true;
                }
            }
            Vector2 local = view->shape.toLocal(screenToPivot(*view, point));
            if (!Rect(0.0f, 0.0f, view->shape.size.x, view->shape.size.y).contains(local))
            {
                return false;
            }
            return view->scrollBy(delta.x, delta.y);
        }
        for (auto it = widget->children().rbegin(); it != widget->children().rend(); ++it)
        {
            if (*it && (*it)->floating && this->routeWheel(it->get(), point, delta))
            {
                return true;
            }
        }
        for (auto it = widget->children().rbegin(); it != widget->children().rend(); ++it)
        {
            if (*it && !(*it)->floating && this->routeWheel(it->get(), point, delta))
            {
                return true;
            }
        }
        return false;
    }

    bool UIWindow::routeNestedCanvas(UIWidget* widget, const Vector2& point, const Vector2& delta)
    {
        if (widget == nullptr || !widget->visible || !liveScale(widget->shape.scale))
        {
            return false;
        }
        if (dynamic_cast<UICanvas*>(widget) != nullptr)
        {
            return this->routeWheel(widget, point, delta);
        }
        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            for (auto it = view->root()->children().rbegin(); it != view->root()->children().rend(); ++it)
            {
                if (*it && this->routeNestedCanvas(it->get(), point, delta))
                {
                    return true;
                }
            }
            return false;
        }
        for (auto it = widget->children().rbegin(); it != widget->children().rend(); ++it)
        {
            if (*it && this->routeNestedCanvas(it->get(), point, delta))
            {
                return true;
            }
        }
        return false;
    }

    void UIWindow::endCanvasDrag(UIWidget* widget)
    {
        if (widget == nullptr)
        {
            return;
        }
        if (UICanvas* canvas = dynamic_cast<UICanvas*>(widget))
        {
            canvas->endDrag();
        }
        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            for (auto& child : view->root()->children())
            {
                this->endCanvasDrag(child.get());
            }
            return;
        }
        for (auto& child : widget->children())
        {
            this->endCanvasDrag(child.get());
        }
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
        mFocused->triggerKeyPress(key, mods);
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
        mFocused->triggerKeyPress(key, mods);
    }

    void UIWindow::setFocus(UIWidget* widget)
    {
        if (mFocused == widget)
        {
            return;
        }
        if (mFocused != nullptr)
        {
            mFocused->focused = false;
            if (mFocused->onLostFocus)
            {
                mFocused->onLostFocus();
            }
        }
        mFocused = widget;
        if (mFocused != nullptr)
        {
            mFocused->focused = true;
            if (mFocused->onGotFocus)
            {
                mFocused->onGotFocus();
            }
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
        const bool active = widget->hovered || widget->pressed;
        widget->hovered = false;
        widget->pressed = false;
        if (active)
        {
            widget->triggerPointerRelease();
        }
        if (UICanvas* canvas = dynamic_cast<UICanvas*>(widget))
        {
            canvas->endDrag();
        }
        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            for (auto& child : view->root()->children())
            {
                this->resetPointerState(child.get());
            }
            return;
        }
        for (auto& child : widget->children())
        {
            this->resetPointerState(child.get());
        }
    }

    bool UIWindow::containsWidget(UIWidget* node, UIWidget* target) const
    {
        if (node == nullptr || target == nullptr)
        {
            return false;
        }
        if (node == target)
        {
            return true;
        }
        if (UIView* view = dynamic_cast<UIView*>(node))
        {
            for (auto& child : view->root()->children())
            {
                if (this->containsWidget(child.get(), target))
                {
                    return true;
                }
            }
            return false;
        }
        for (auto& child : node->children())
        {
            if (this->containsWidget(child.get(), target))
            {
                return true;
            }
        }
        return false;
    }

    bool UIWindow::focusAlive()
    {
        if (mFocused == nullptr)
        {
            return false;
        }
        if (this->containsWidget(mRoot.get(), mFocused))
        {
            return true;
        }
        mFocused->focused = false;
        if (mFocused->onLostFocus)
        {
            mFocused->onLostFocus();
        }
        mFocused = nullptr;
        return false;
    }
}
