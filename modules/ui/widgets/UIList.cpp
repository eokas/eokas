#include "UIList.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    void UIList::addChild(const std::shared_ptr<UIWidget>& child)
    {
        child->parent = this;
        children.push_back(child);
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
            this->resize(Vector2(main, shape.size.y));
        }
        else
        {
            this->resize(Vector2(shape.size.x, main));
        }
    }

    void UIList::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
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
            this->placeChild(*child, Vector2(floorf(cursor.x + 0.5f), floorf(cursor.y + 0.5f)));
            if (direction == UIDirection::Horizontal)
            {
                cursor.x += child->shape.size.x;
            }
            else
            {
                cursor.y += child->shape.size.y;
            }
        }
        if (color.a > 0.0f)
        {
            primitive.addQuad(worldTrans(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }
        UIWidget::render(primitive);
    }
}
