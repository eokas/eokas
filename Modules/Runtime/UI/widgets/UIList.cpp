#include "UIList.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    void UIList::refit()
    {
        float main = padding;
        bool any = false;
        for (auto& child : children())
        {
            if (!child || !child->visible)
            {
                continue;
            }
            if (any)
            {
                main += spacing;
            }
            any = true;
            if (direction == UIDirection::Horizontal)
            {
                main += child->shape.size.x;
            }
            else
            {
                main += child->shape.size.y;
            }
        }
        main += padding;
        if (direction == UIDirection::Horizontal)
        {
            this->resize(Vector2(main, shape.size.y));
        }
        else
        {
            this->resize(Vector2(shape.size.x, main));
        }
    }

    void UIList::layout()
    {
        if (!visible)
        {
            return;
        }
        float main = padding;
        float cross = 0.0f;
        bool any = false;
        for (auto& child : children())
        {
            if (!child || !child->visible)
            {
                continue;
            }
            child->layout();
            float childMain = direction == UIDirection::Horizontal ? child->shape.size.x : child->shape.size.y;
            float childCross = direction == UIDirection::Horizontal ? child->shape.size.y : child->shape.size.x;
            if (childCross > cross)
            {
                cross = childCross;
            }
            if (any)
            {
                main += spacing;
            }
            any = true;
            main += childMain;
        }
        main += padding;
        float width = shape.size.x;
        float height = shape.size.y;
        bool grow = false;
        if (direction == UIDirection::Horizontal)
        {
            if (width <= 0.0f)
            {
                width = main;
                grow = true;
            }
            if (height <= 0.0f)
            {
                height = cross + padding * 2.0f;
                grow = true;
            }
        }
        else
        {
            if (height <= 0.0f)
            {
                height = main;
                grow = true;
            }
            if (width <= 0.0f)
            {
                width = cross + padding * 2.0f;
                grow = true;
            }
        }
        if (grow)
        {
            Vector2 topLeft = shape.origin - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
            shape.setBox(topLeft, Vector2(width, height));
        }
        Vector2 cursor(padding, padding);
        bool first = true;
        for (auto& child : children())
        {
            if (!child || !child->visible)
            {
                continue;
            }
            if (!first)
            {
                if (direction == UIDirection::Horizontal)
                {
                    cursor.x += spacing;
                }
                else
                {
                    cursor.y += spacing;
                }
            }
            first = false;
            float x = cursor.x;
            float y = cursor.y;
            if (centerCross && direction == UIDirection::Horizontal)
            {
                y = (shape.size.y - child->shape.size.y) * 0.5f;
                if (y < 0.0f)
                {
                    y = 0.0f;
                }
            }
            this->placeChild(*child, Vector2(floorf(x + 0.5f), floorf(y + 0.5f)));
            if (direction == UIDirection::Horizontal)
            {
                cursor.x += child->shape.size.x;
            }
            else
            {
                cursor.y += child->shape.size.y;
            }
        }
    }

    void UIList::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        if (color.a > 0.0f)
        {
            primitive.addQuad(matrixLocalToScreen(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }
        for (auto& child : children())
        {
            if (!child || child->overlay || !child->visible)
            {
                continue;
            }
            bool saved = child->deferOverlay;
            child->deferOverlay = true;
            if (!primitive.outsideClip(child->screenBounds()))
            {
                child->render(primitive);
            }
            child->deferOverlay = saved;
        }
        if (!deferOverlay)
        {
            this->renderOverlay(primitive);
        }
    }
}
