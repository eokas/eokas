#include "UIMenu.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    UIMenuItem::UIMenuItem()
    {
        this->setContent(std::make_shared<UIText>());
    }

    void UIMenuItem::setContent(const std::shared_ptr<UIWidget>& widget)
    {
        content = widget;
        this->detachChildren();
        if (content)
        {
            this->attachChild(content);
        }
    }

    void UIMenuItem::setText(const String& text)
    {
        UIText* t = this->label();
        if (t == nullptr)
        {
            auto created = std::make_shared<UIText>();
            this->setContent(created);
            t = created.get();
        }
        t->text = text;
    }

    UIText* UIMenuItem::label() const
    {
        return dynamic_cast<UIText*>(content.get());
    }

    void UIMenuItem::syncSize()
    {
        if (!content)
        {
            return;
        }
        content->layout();
        this->resize(Vector2(content->shape.size.x + paddingX * 2.0f, content->shape.size.y + paddingY * 2.0f));
    }

    void UIMenuItem::layout()
    {
        if (!visible)
        {
            return;
        }
        if (content)
        {
            Vector2 pos(floorf(paddingX + 0.5f), floorf(paddingY + 0.5f));
            this->placeChild(*content, pos);
        }
    }

    void UIMenuItem::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Color bg = isHovered() ? hoverFill : background;
        primitive.addQuad(matrixLocalToScreen(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
        UIWidget::render(primitive);
    }

    UIMenu::UIMenu()
    {
        list = std::make_shared<UIList>();
        list->direction = direction;
        list->padding = padding;
        list->spacing = spacing;
        list->color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        this->attachChild(list);
    }

    void UIMenu::addItem(const std::shared_ptr<UIMenuItem>& item)
    {
        if (list && item)
        {
            list->addChild(item);
        }
    }

    void UIMenu::layout()
    {
        if (!visible)
        {
            return;
        }
        if (list)
        {
            list->direction = direction;
            list->padding = padding;
            list->spacing = spacing;

            float maxItemWidth = 0.0f;
            float maxItemHeight = 0.0f;
            float sumItemHeight = 0.0f;
            int itemCount = 0;
            for (auto& child : list->children())
            {
                UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
                if (item == nullptr || !item->visible)
                {
                    continue;
                }
                item->syncSize();
                if (item->shape.size.x > maxItemWidth)
                {
                    maxItemWidth = item->shape.size.x;
                }
                if (item->shape.size.y > maxItemHeight)
                {
                    maxItemHeight = item->shape.size.y;
                }
                sumItemHeight += item->shape.size.y;
                itemCount += 1;
            }

            float gap = itemCount > 1 ? spacing * (float)(itemCount - 1) : 0.0f;
            if (direction == UIDirection::Horizontal)
            {
                this->resize(Vector2(shape.size.x, padding * 2.0f + maxItemHeight));
            }
            else
            {
                this->resize(Vector2(padding * 2.0f + maxItemWidth, padding * 2.0f + sumItemHeight + gap));
            }

            for (auto& child : list->children())
            {
                UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
                if (item == nullptr || !item->visible)
                {
                    continue;
                }
                if (direction == UIDirection::Horizontal)
                {
                    item->resize(Vector2(item->shape.size.x, maxItemHeight));
                }
                else
                {
                    item->resize(Vector2(maxItemWidth, item->shape.size.y));
                }
            }

            this->placeChild(*list, Vector2::ZERO, shape.size);
        }
        UIWidget::layout();
    }

    void UIMenu::render(UIPrimitive& primitive)
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
}
