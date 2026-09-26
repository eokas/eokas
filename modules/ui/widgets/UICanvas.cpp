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

        float axisScale(float value)
        {
            return value == 0.0f ? 1.0f : value;
        }
    }

    UICanvas::UICanvas()
    {
        pickable = true;
        dragable = true;
    }

    void UICanvas::scaleAt(const Vector2& pivot, float value)
    {
        float next = clampScale(value, minScale, maxScale);
        float kx = next / axisScale(localScale.x);
        float ky = next / axisScale(localScale.y);
        rect.origin.x = pivot.x - (pivot.x - rect.origin.x) * kx;
        rect.origin.y = pivot.y - (pivot.y - rect.origin.y) * ky;
        localScale = Vector2(next, next);
        this->refit();
    }

    void UICanvas::addChild(const std::shared_ptr<UIWidget>& child)
    {
        children.push_back(child);
    }

    void UICanvas::layout(const Rect& given)
    {
        rect.origin = given.origin;
        for (auto& child : children)
        {
            if (child)
            {
                child->layout(child->rect);
            }
        }
        this->refit();
    }

    void UICanvas::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        if (color.a > 0.0f)
        {
            primitive.addQuad(rect, UIFont::solidUV(), color);
        }
        primitive.popOrigin();
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
            expand(minX, minY, maxX, maxY, any, child->finalRect());
        }
        if (!any)
        {
            rect.size = Vector2::ZERO;
            return;
        }
        if (minX != 0.0f || minY != 0.0f)
        {
            rect.origin += Vector2(minX, minY) * localScale;
            Vector2 shift(minX, minY);
            for (auto& child : children)
            {
                if (child)
                {
                    child->rect.origin -= shift;
                }
            }
            maxX -= minX;
            maxY -= minY;
        }
        rect.size = Vector2(maxX, maxY);
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
            mGrab = local - widget->rect.origin;
            return;
        }
        widget->rect.origin = local - mGrab;
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

    void UICanvas::triggerPointerDrag(float x, float y, int button)
    {
        if (button != 0 || !dragable)
        {
            UIWidget::triggerPointerDrag(x, y, button);
            return;
        }
        Vector2 local(x, y);
        if (!mSelfDrag)
        {
            mSelfDrag = true;
            mGrab = local;
            UIWidget::triggerPointerDrag(x, y, button);
            return;
        }
        rect.origin += local - mGrab;
        mGrab = local;
        this->refit();
        UIWidget::triggerPointerDrag(x, y, button);
    }

    void UICanvas::triggerPointerRelease()
    {
        this->endDrag();
        UIWidget::triggerPointerRelease();
    }
}
