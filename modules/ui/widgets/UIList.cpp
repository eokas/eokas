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
        float cursorX = rect.x + padding;
        float cursorY = rect.y + padding;
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
                    cursorX += spacing;
                }
                else
                {
                    cursorY += spacing;
                }
            }
            first = false;
            float width = child->rect.width;
            float height = child->rect.height;
            child->layout(Rect(floorf(cursorX + 0.5f), floorf(cursorY + 0.5f), width, height));
            if (direction == UIDirection::Horizontal)
            {
                cursorX += child->rect.width;
            }
            else
            {
                cursorY += child->rect.height;
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
                main += child->rect.width;
            }
            else
            {
                main += child->rect.height;
            }
        }
        main += padding;
        if (direction == UIDirection::Horizontal)
        {
            rect.width = main;
        }
        else
        {
            rect.height = main;
        }
    }

    void UIList::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        if (color.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), color);
        }
        UIWidget::render(shape);
    }
}
