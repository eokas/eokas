#include "UIFrame.h"
#include "widgets/UICanvas.h"
#include "widgets/UIChart.h"
#include "widgets/UIView.h"

#include <cmath>
#include <cstddef>

namespace eokas
{
    namespace
    {
        constexpr const char* kUIShaderPath = "../shaders/UI.hlsl";

        bool liveScale(const Vector2& scale)
        {
            return scale.x != 0.0f && scale.y != 0.0f;
        }

        Vector2 parentPoint(const Vector2& point, const Vector2& origin, const Vector2& scale)
        {
            return Vector2(
                (point.x - origin.x) / scale.x,
                (point.y - origin.y) / scale.y);
        }

        Vector2 layoutPoint(const UIWidget* widget, const Vector2& parentLocal)
        {
            return widget->rect.origin + Vector2(
                (parentLocal.x - widget->rect.origin.x) / widget->localScale.x,
                (parentLocal.y - widget->rect.origin.y) / widget->localScale.y);
        }
    }

    void UIFrame::init(uint32_t width, uint32_t height)
    {
        mWidth = (float)width;
        mHeight = (float)height;
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mPressX = 0.0f;
        mPressY = 0.0f;
        mDragged = false;
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

    void UIFrame::quit()
    {
        this->setFocus(nullptr);
        this->resetPointerState(mRoot.get());
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mPressX = 0.0f;
        mPressY = 0.0f;
        mDragged = false;
        mRoot.reset();
        if (mPrimitive)
        {
            mPrimitive->material.reset();
        }
        mPrimitive.reset();
    }

    const std::shared_ptr<UIWidget>& UIFrame::root() const
    {
        return mRoot;
    }

    void UIFrame::setRoot(const std::shared_ptr<UIWidget>& widget)
    {
        this->setFocus(nullptr);
        this->resetPointerState(mRoot.get());
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mPressX = 0.0f;
        mPressY = 0.0f;
        mDragged = false;
        mRoot = widget;
    }

    void UIFrame::flush()
    {
        if (!mPrimitive)
        {
            return;
        }

        mPrimitive->begin();
        if (mRoot)
        {
            mRoot->layout(mRoot->rect);
            mRoot->render(*mPrimitive);
        }
        mPrimitive->end();
    }

    UIPrimitive::Ref UIFrame::primitive() const
    {
        return mPrimitive;
    }

    UIWidget* UIFrame::hitTest(float x, float y)
    {
        if (!mRoot)
        {
            return nullptr;
        }
        mRoot->layout(mRoot->rect);
        return mRoot->pick(Vector2(x, y));
    }

    void UIFrame::onMouseMove(float x, float y)
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
        if (hit == mHovered)
        {
            return;
        }
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

    void UIFrame::onMouseDown(float x, float y, int button)
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
            if (mPressed->acceptsKeyFocus())
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

    void UIFrame::onMouseUp(float x, float y, int button)
    {
        UIWidget* hit = this->hitTest(x, y);
        UIWidget* pressed = mPressed;
        if (pressed != nullptr && button == mPressedButton)
        {
            this->endCanvasDrag(mRoot.get());
            pressed->triggerPointerRelease();
            UICanvas* canvas = nullptr;
            this->dragTargetOf(pressed, canvas);
            if (!mDragged && hit == pressed && button == 0)
            {
                pressed->triggerClick();
                if (canvas != nullptr)
                {
                    if (UIChart* chart = dynamic_cast<UIChart*>(pressed))
                    {
                        canvas->select(chart);
                    }
                }
            }
            else if (mDragged && canvas != nullptr)
            {
                if (UIChart* chart = dynamic_cast<UIChart*>(pressed))
                {
                    float dropX = x;
                    float dropY = y;
                    Vector2 origin = Vector2::ZERO;
                    Vector2 scale(1.0f, 1.0f);
                    if (this->findWidget(mRoot.get(), canvas, Vector2::ZERO, Vector2(1.0f, 1.0f), origin, scale) && liveScale(scale))
                    {
                        Vector2 local = parentPoint(Vector2(x, y), origin, scale);
                        dropX = local.x;
                        dropY = local.y;
                    }
                    canvas->dispatchDrop(chart, hit, dropX, dropY);
                }
            }
        }
        mPressed = nullptr;
        mPressedButton = -1;
        mDragged = false;
        this->onMouseMove(x, y);
    }

    void UIFrame::onMouseWheel(float x, float y, float deltaX, float deltaY)
    {
        this->routeWheel(mRoot.get(), Vector2(x, y), Vector2::ZERO, Vector2(1.0f, 1.0f), Vector2(deltaX, deltaY));
    }

    void UIFrame::dispatchDrag(float x, float y)
    {
        if (mPressed == nullptr)
        {
            return;
        }
        UICanvas* canvas = nullptr;
        UIWidget* target = this->dragTargetOf(mPressed, canvas);
        if (canvas != nullptr && target != nullptr && target != canvas)
        {
            Vector2 origin = Vector2::ZERO;
            Vector2 scale(1.0f, 1.0f);
            if (this->findWidget(mRoot.get(), target, Vector2::ZERO, Vector2(1.0f, 1.0f), origin, scale) && liveScale(scale))
            {
                Vector2 local = parentPoint(Vector2(x, y), origin, scale);
                canvas->dragChild(target, local.x, local.y);
                return;
            }
        }
        Vector2 origin = Vector2::ZERO;
        Vector2 scale(1.0f, 1.0f);
        this->findWidget(mRoot.get(), mPressed, Vector2::ZERO, Vector2(1.0f, 1.0f), origin, scale);
        Vector2 local = Vector2::ZERO;
        if (liveScale(scale))
        {
            local = parentPoint(Vector2(x, y), origin, scale);
        }
        mPressed->triggerPointerDrag(local.x, local.y, mPressedButton);
    }

    bool UIFrame::collectPath(UIWidget* node, UIWidget* target, std::vector<UIWidget*>& path) const
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
            for (auto& child : view->root()->children)
            {
                if (this->collectPath(child.get(), target, path))
                {
                    return true;
                }
            }
            path.pop_back();
            return false;
        }
        for (auto& child : node->children)
        {
            if (this->collectPath(child.get(), target, path))
            {
                return true;
            }
        }
        path.pop_back();
        return false;
    }

    UIWidget* UIFrame::dragTargetOf(UIWidget* pressed, UICanvas*& canvas) const
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

    bool UIFrame::findWidget(UIWidget* node, UIWidget* target, const Vector2& origin, const Vector2& scale, Vector2& outOrigin, Vector2& outScale) const
    {
        if (node == nullptr)
        {
            return false;
        }
        if (node == target)
        {
            outOrigin = origin;
            outScale = scale;
            return true;
        }
        Vector2 nextOrigin = origin + scale * node->rect.origin;
        Vector2 nextScale = scale * node->localScale;
        if (UIView* view = dynamic_cast<UIView*>(node))
        {
            Vector2 contentOrigin = nextOrigin + nextScale * view->root()->rect.origin;
            Vector2 contentScale = nextScale * view->root()->localScale;
            for (auto& child : view->root()->children)
            {
                if (this->findWidget(child.get(), target, contentOrigin, contentScale, outOrigin, outScale))
                {
                    return true;
                }
            }
            return false;
        }
        for (auto& child : node->children)
        {
            if (this->findWidget(child.get(), target, nextOrigin, nextScale, outOrigin, outScale))
            {
                return true;
            }
        }
        return false;
    }

    bool UIFrame::routeWheel(UIWidget* widget, const Vector2& point, const Vector2& origin, const Vector2& scale, const Vector2& delta)
    {
        if (widget == nullptr || !widget->visible || !liveScale(scale))
        {
            return false;
        }
        if (UICanvas* canvas = dynamic_cast<UICanvas*>(widget))
        {
            Vector2 nextOrigin = origin + scale * canvas->rect.origin;
            Vector2 nextScale = scale * canvas->localScale;
            for (auto it = canvas->children.rbegin(); it != canvas->children.rend(); ++it)
            {
                if (*it && this->routeNestedCanvas(it->get(), point, nextOrigin, nextScale, delta))
                {
                    return true;
                }
            }
            if (!liveScale(canvas->localScale))
            {
                return false;
            }
            Vector2 parentLocal = parentPoint(point, origin, scale);
            Vector2 layout = layoutPoint(canvas, parentLocal);
            if (!canvas->rect.contains(layout))
            {
                return false;
            }
            UIWidget* hit = canvas->pick(parentLocal);
            if (UIChart* chart = dynamic_cast<UIChart*>(hit))
            {
                Vector2 chartOrigin = Vector2::ZERO;
                Vector2 chartScale(1.0f, 1.0f);
                if (this->findWidget(mRoot.get(), chart, Vector2::ZERO, Vector2(1.0f, 1.0f), chartOrigin, chartScale) && liveScale(chartScale))
                {
                    float current = chart->localScale.x == 0.0f ? 1.0f : chart->localScale.x;
                    float factor = expf(-delta.y * chart->scaleSensitivity);
                    chart->scaleAt(parentPoint(point, chartOrigin, chartScale), current * factor);
                    return true;
                }
            }
            float current = canvas->localScale.x;
            if (current == 0.0f)
            {
                current = 1.0f;
            }
            float factor = expf(-delta.y * canvas->scaleSensitivity);
            canvas->scaleAt(parentLocal, current * factor);
            return true;
        }
        Vector2 nextOrigin = origin + scale * widget->rect.origin;
        Vector2 nextScale = scale * widget->localScale;
        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            Vector2 contentOrigin = nextOrigin + nextScale * view->root()->rect.origin;
            Vector2 contentScale = nextScale * view->root()->localScale;
            const std::shared_ptr<UIWidget>& content = view->root();
            for (auto it = content->children.rbegin(); it != content->children.rend(); ++it)
            {
                if (*it && (*it)->floating && this->routeWheel(it->get(), point, contentOrigin, contentScale, delta))
                {
                    return true;
                }
            }
            for (auto it = content->children.rbegin(); it != content->children.rend(); ++it)
            {
                if (*it && !(*it)->floating && this->routeWheel(it->get(), point, contentOrigin, contentScale, delta))
                {
                    return true;
                }
            }
            if (!liveScale(widget->localScale))
            {
                return false;
            }
            Vector2 layout = layoutPoint(widget, parentPoint(point, origin, scale));
            if (!view->rect.contains(layout))
            {
                return false;
            }
            return view->scrollBy(delta.x, delta.y);
        }
        for (auto it = widget->children.rbegin(); it != widget->children.rend(); ++it)
        {
            if (*it && (*it)->floating && this->routeWheel(it->get(), point, nextOrigin, nextScale, delta))
            {
                return true;
            }
        }
        for (auto it = widget->children.rbegin(); it != widget->children.rend(); ++it)
        {
            if (*it && !(*it)->floating && this->routeWheel(it->get(), point, nextOrigin, nextScale, delta))
            {
                return true;
            }
        }
        return false;
    }

    bool UIFrame::routeNestedCanvas(UIWidget* widget, const Vector2& point, const Vector2& origin, const Vector2& scale, const Vector2& delta)
    {
        if (widget == nullptr || !widget->visible || !liveScale(scale))
        {
            return false;
        }
        if (dynamic_cast<UICanvas*>(widget) != nullptr)
        {
            return this->routeWheel(widget, point, origin, scale, delta);
        }
        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            if (!liveScale(view->localScale))
            {
                return false;
            }
            Vector2 nextOrigin = origin + scale * view->rect.origin;
            Vector2 nextScale = scale * view->localScale;
            Vector2 contentOrigin = nextOrigin + nextScale * view->root()->rect.origin;
            Vector2 contentScale = nextScale * view->root()->localScale;
            for (auto it = view->root()->children.rbegin(); it != view->root()->children.rend(); ++it)
            {
                if (*it && this->routeNestedCanvas(it->get(), point, contentOrigin, contentScale, delta))
                {
                    return true;
                }
            }
            return false;
        }
        Vector2 nextOrigin = origin + scale * widget->rect.origin;
        Vector2 nextScale = scale * widget->localScale;
        for (auto it = widget->children.rbegin(); it != widget->children.rend(); ++it)
        {
            if (*it && this->routeNestedCanvas(it->get(), point, nextOrigin, nextScale, delta))
            {
                return true;
            }
        }
        return false;
    }

    void UIFrame::endCanvasDrag(UIWidget* widget)
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
            for (auto& child : view->root()->children)
            {
                this->endCanvasDrag(child.get());
            }
            return;
        }
        for (auto& child : widget->children)
        {
            this->endCanvasDrag(child.get());
        }
    }

    void UIFrame::onChar(uint32_t codepoint)
    {
        if (!this->focusAlive())
        {
            return;
        }
        mFocused->triggerChar(codepoint);
    }

    void UIFrame::onKeyDown(UIKey key, const UIKeyMods& mods)
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
        mFocused->triggerKey(key, mods);
    }

    void UIFrame::setFocus(UIWidget* widget)
    {
        if (mFocused == widget)
        {
            return;
        }
        if (mFocused != nullptr)
        {
            mFocused->triggerBlur();
        }
        mFocused = widget;
        if (mFocused != nullptr)
        {
            mFocused->triggerFocus();
        }
    }

    UIWidget* UIFrame::focus() const
    {
        return mFocused;
    }

    void UIFrame::resetPointerState(UIWidget* widget)
    {
        if (widget == nullptr)
        {
            return;
        }
        widget->resetPointerState();
        if (UICanvas* canvas = dynamic_cast<UICanvas*>(widget))
        {
            canvas->endDrag();
        }
        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            for (auto& child : view->root()->children)
            {
                this->resetPointerState(child.get());
            }
            return;
        }
        for (auto& child : widget->children)
        {
            this->resetPointerState(child.get());
        }
    }

    bool UIFrame::containsWidget(UIWidget* node, UIWidget* target) const
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
            for (auto& child : view->root()->children)
            {
                if (this->containsWidget(child.get(), target))
                {
                    return true;
                }
            }
            return false;
        }
        for (auto& child : node->children)
        {
            if (this->containsWidget(child.get(), target))
            {
                return true;
            }
        }
        return false;
    }

    bool UIFrame::focusAlive()
    {
        if (mFocused == nullptr)
        {
            return false;
        }
        if (this->containsWidget(mRoot.get(), mFocused))
        {
            return true;
        }
        mFocused->triggerBlur();
        mFocused = nullptr;
        return false;
    }
}
