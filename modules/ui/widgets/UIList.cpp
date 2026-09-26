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
        this->rect = rect;
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
            child->layout(Rect(Vector2(floorf(cursor.x + 0.5f), floorf(cursor.y + 0.5f)), child->rect.size));
            if (direction == UIDirection::Horizontal)
            {
                cursor.x += child->rect.size.x;
            }
            else
            {
                cursor.y += child->rect.size.y;
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
                main += child->rect.size.x;
            }
            else
            {
                main += child->rect.size.y;
            }
        }
        main += padding;
        if (direction == UIDirection::Horizontal)
        {
            rect.size.x = main;
        }
        else
        {
            rect.size.y = main;
        }
    }

    void UIList::render(UIPrimitive& primitive)
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
}
