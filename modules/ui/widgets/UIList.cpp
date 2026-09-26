#include "UIList.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    void UIList::addChild(const std::shared_ptr<UIWidget>& child)
    {
        children.push_back(child);
    }

    void UIList::layout(const Rect& rect)
    {
        {
            Rect _box = rect;
            shape.size = _box.size;
            shape.origin = _box.origin + shape.pivot * shape.size;
        }
        Vector2 cursor(padding, padding);
        bool first = true;
        for (auto& child : children)
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
            child->layout(Rect(Vector2(floorf(cursor.x + 0.5f), floorf(cursor.y + 0.5f)), child->shape.size));
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

    void UIList::refit()
    {
        float main = padding;
        bool any = false;
        for (auto& child : children)
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
            shape.origin.x += shape.pivot.x * ((main) - shape.size.x);
            shape.size.x = main;
        }
        else
        {
            shape.origin.y += shape.pivot.y * ((main) - shape.size.y);
            shape.size.y = main;
        }
    }

    void UIList::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(shape.origin, shape.scale);
        if (color.a > 0.0f)
        {
            primitive.addQuad(Rect(shape.left(), shape.top(), shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
