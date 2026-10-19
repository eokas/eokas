#include "UICanvas.h"
#include "UIChart.h"
#include "../UIFont.h"

#include <cmath>

namespace eokas
{
    namespace
    {
        float lesser(float a, float b)
        {
            return a < b ? a : b;
        }

        float greater(float a, float b)
        {
            return a > b ? a : b;
        }

        void expand(float& minX, float& minY, float& maxX, float& maxY, bool& any, const Rect& box)
        {
            float x0 = box.origin.x;
            float y0 = box.origin.y;
            float x1 = box.origin.x + box.size.x;
            float y1 = box.origin.y + box.size.y;
            if (x1 < x0)
            {
                float swap = x0;
                x0 = x1;
                x1 = swap;
            }
            if (y1 < y0)
            {
                float swap = y0;
                y0 = y1;
                y1 = swap;
            }
            if (!any)
            {
                minX = x0;
                minY = y0;
                maxX = x1;
                maxY = y1;
                any = true;
                return;
            }
            minX = lesser(minX, x0);
            minY = lesser(minY, y0);
            maxX = greater(maxX, x1);
            maxY = greater(maxY, y1);
        }

        float clampScale(float value, float minScale, float maxScale)
        {
            float lo = minScale;
            float hi = maxScale;
            if (lo < 0.01f)
            {
                lo = 0.01f;
            }
            if (hi < lo)
            {
                hi = lo;
            }
            if (value < lo)
            {
                return lo;
            }
            if (value > hi)
            {
                return hi;
            }
            return value;
        }

        bool liveScale(const Vector2& scale)
        {
            return scale.x != 0.0f && scale.y != 0.0f;
        }

        Vector2 screenToParentPivot(const UIWidget& widget, const Vector2& screen)
        {
            if (widget.parent() == nullptr)
            {
                return screen;
            }
            return widget.parent()->screenToPivot(screen);
        }
    }

    UICanvas::UICanvas()
    {
        pickable = true;
        dragable = true;
    }

    void UICanvas::scaleAt(const Vector2& focal, float value)
    {
        float next = clampScale(value, minScale, maxScale);
        shape.setScaleAround(focal, Vector2(next, next));
        this->refit();
    }

    void UICanvas::addChild(const std::shared_ptr<UIWidget>& child)
    {
        this->attachChild(child);
    }

    void UICanvas::layout()
    {
        if (!visible)
        {
            return;
        }
        UIWidget::layout();
        this->refit();
    }

    void UICanvas::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        if (color.a > 0.0f)
        {
            primitive.addQuad(matrixLocalToScreen(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }
        UIWidget::render(primitive);
    }

    void UICanvas::refit()
    {
        bool any = false;
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        for (auto& child : children())
        {
            if (!child || !child->visible)
            {
                continue;
            }
            expand(minX, minY, maxX, maxY, any, child->shape.bounds(child->shape.localTrans()));
        }
        Vector2 topLeft(-shape.pivot.x * shape.size.x, -shape.pivot.y * shape.size.y);
        if (!any)
        {
            shape.setBox(topLeft, Vector2::ZERO);
            return;
        }
        Vector2 delta = topLeft - Vector2(minX, minY);
        if (delta.x != 0.0f || delta.y != 0.0f)
        {
            shape.origin -= UIShape::transformVector(shape.localTrans(), delta);
            for (auto& child : children())
            {
                if (child)
                {
                    child->shape.origin += delta;
                }
            }
        }
        this->resize(Vector2(maxX - minX, maxY - minY));
    }

    void UICanvas::dragChild(UIWidget* widget, float localX, float localY)
    {
        if (widget == nullptr || widget == this)
        {
            return;
        }
        Vector2 local(localX, localY);
        if (mDragWidget != widget)
        {
            mDragWidget = widget;
            Vector2 corner = widget->shape.origin - Vector2(
                widget->shape.pivot.x * widget->shape.size.x,
                widget->shape.pivot.y * widget->shape.size.y);
            mGrab = local - corner;
            return;
        }
        widget->shape.setBox(local - mGrab, widget->shape.size);
        this->refit();
    }

    void UICanvas::endDrag()
    {
        mDragWidget = nullptr;
        mSelfDrag = false;
    }

    UIWidget* UICanvas::findPressedDragable(UIWidget* node) const
    {
        if (node == nullptr)
        {
            return nullptr;
        }
        UIWidget* found = nullptr;
        for (auto& child : node->children())
        {
            if (UIWidget* deeper = this->findPressedDragable(child.get()))
            {
                found = deeper;
            }
        }
        if (node != this && node->isPressed() && node->dragable)
        {
            return node;
        }
        return found;
    }

    void UICanvas::applyCanvasSelection(UIWidget* selectedItem) const
    {
        for (const auto& child : children())
        {
            if (!child)
            {
                continue;
            }
            UIChart* chart = dynamic_cast<UIChart*>(child.get());
            if (chart != nullptr)
            {
                chart->selected = (child.get() == selectedItem);
            }
        }
    }

    bool UICanvas::handlePointerMove(float screenX, float screenY, const Vector2& delta)
    {
        Vector2 position = screenToParentPivot(*this, Vector2(screenX, screenY));
        if (!isPressed() || !dragable)
        {
            return UIWidget::handlePointerMove(screenX, screenY, delta);
        }
        if (!mSelfDrag)
        {
            mSelfDrag = true;
            mGrab = position;
            return UIWidget::handlePointerMove(screenX, screenY, delta);
        }
        shape.origin += position - mGrab;
        mGrab = position;
        this->refit();
        UIWidget::handlePointerMove(screenX, screenY, delta);
        return true;
    }

    bool UICanvas::handlePointerRelease(float screenX, float screenY, int button)
    {
        this->endDrag();
        return UIWidget::handlePointerRelease(screenX, screenY, button);
    }

    bool UICanvas::handleDrag(float screenX, float screenY, const Vector2& delta)
    {
        UIWidget* target = this->findPressedDragable(this);
        if (target != nullptr && target != this)
        {
            Vector2 parentPivot = screenToParentPivot(*target, Vector2(screenX, screenY));
            this->dragChild(target, parentPivot.x, parentPivot.y);
            return true;
        }
        return UIWidget::handleDrag(screenX, screenY, delta);
    }

    bool UICanvas::handleDrop(float screenX, float screenY, UIWidget* hitUnderCursor, const Vector2& delta)
    {
        (void)screenX;
        (void)screenY;
        (void)hitUnderCursor;
        return UIWidget::handleDrop(screenX, screenY, hitUnderCursor, delta);
    }

    bool UICanvas::routeNestedWheel(float screenX, float screenY, float deltaX, float deltaY, UIWidget* widget) const
    {
        if (widget == nullptr || !widget->visible || !liveScale(widget->shape.scale))
        {
            return false;
        }
        return widget->handleWheel(screenX, screenY, deltaX, deltaY);
    }

    bool UICanvas::handleWheel(float screenX, float screenY, float deltaX, float deltaY)
    {
        if (!visible || !liveScale(shape.scale))
        {
            return false;
        }
        Vector2 point(screenX, screenY);
        for (auto it = children().rbegin(); it != children().rend(); ++it)
        {
            if (*it && this->routeNestedWheel(screenX, screenY, deltaX, deltaY, it->get()))
            {
                return true;
            }
        }
        Vector2 local = this->screenToLocal(point);
        if (!Rect(0.0f, 0.0f, shape.size.x, shape.size.y).contains(local))
        {
            return false;
        }
        Vector2 parentPivot = screenToParentPivot(*this, point);
        if (UIWidget* hit = this->pick(parentPivot))
        {
            if (hit != this && hit->handleWheel(screenX, screenY, deltaX, deltaY))
            {
                return true;
            }
        }
        float current = shape.scale.x;
        if (current == 0.0f)
        {
            current = 1.0f;
        }
        float factor = expf(-deltaY * scaleSensitivity);
        this->scaleAt(parentPivot, current * factor);
        return true;
    }

    void UICanvas::endActiveDrag()
    {
        this->endDrag();
    }

}