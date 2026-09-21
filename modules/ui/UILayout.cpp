#include "UILayout.h"
#include "UIFont.h"
#include <cmath>

namespace eokas
{
    void UILayout::addChild(const std::shared_ptr<UIWidget>& child)
    {
        children.push_back(child);
    }

    void UILayout::layout()
    {
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
                if (direction == UILayoutDirection::Horizontal)
                {
                    cursorX += spacing;
                }
                else
                {
                    cursorY += spacing;
                }
            }
            first = false;
            child->rect.x = floorf(cursorX + 0.5f);
            child->rect.y = floorf(cursorY + 0.5f);
            if (direction == UILayoutDirection::Horizontal)
            {
                cursorX += child->rect.width;
            }
            else
            {
                cursorY += child->rect.height;
            }
        }
    }

    void UILayout::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        layout();
        if (color.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), color);
        }
        UIWidget::render(shape);
    }
}
