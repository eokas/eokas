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
        child->parent = this;
        children.push_back(child);
    }

    void UICanvas::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        this->refit();
        if (color.a > 0.0f)
        {
            primitive.addQuad(worldTrans(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), color);
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
        for (auto& child : children)
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
            for (auto& child : children)
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

    void UICanvas::dispatchDrop(UIChart* source, UIWidget* hit, float x, float y)
    {
        if (source == nullptr || !source->onDrop)
        {
            return;
        }
        UIChart* target = dynamic_cast<UIChart*>(hit);
        if (target == source)
        {
            target = nullptr;
        }
        source->onDrop(target, x, y);
    }

    namespace
    {
        void applySelect(UIWidget* node, UIChart* chart)
        {
            if (node == nullptr)
            {
                return;
            }
            if (UIChart* item = dynamic_cast<UIChart*>(node))
            {
                item->selected = item == chart;
            }
            for (auto& child : node->children)
            {
                applySelect(child.get(), chart);
            }
        }
    }

    void UICanvas::select(UIChart* chart)
    {
        for (auto& child : children)
        {
            applySelect(child.get(), chart);
        }
    }

    void UICanvas::triggerPointerMove(const Vector2& position, const Vector2& delta)
    {
        (void)delta;
        if (!pressed || !dragable)
        {
            UIWidget::triggerPointerMove(position, delta);
            return;
        }
        Vector2 local = position;
        if (!mSelfDrag)
        {
            mSelfDrag = true;
            mGrab = local;
            UIWidget::triggerPointerMove(position, delta);
            return;
        }
        shape.origin += local - mGrab;
        mGrab = local;
        this->refit();
        UIWidget::triggerPointerMove(position, delta);
    }

    void UICanvas::triggerPointerRelease()
    {
        this->endDrag();
        UIWidget::triggerPointerRelease();
    }
}
