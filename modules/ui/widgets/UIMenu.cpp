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
        children.clear();
        if (content)
        {
            children.push_back(content);
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

        if (UIText* t = this->label())
        {
            UIFont* font = UIFont::find(t->style.fontPath);
            if (font != nullptr && font->isOpen())
            {
                float bake = (float)font->pixelSize();
                float scale = (t->style.fontSize > 0.0f ? t->style.fontSize : bake) / bake;
                float width = 0.0f;
                for (size_t i = 0; i < t->text.length(); i++)
                {
                    width += font->glyph(t->text.at(i)).advance * scale;
                }
                t->rect.size.x = floorf(width + 0.5f);
                float tight = font->ascender() - font->descender();
                t->rect.size.y = floorf(tight * scale + 0.5f);
            }
            rect.size.x = t->rect.size.x + paddingX * 2.0f;
            rect.size.y = t->rect.size.y + paddingY * 2.0f;
            return;
        }

        rect.size.x = content->rect.size.x + paddingX * 2.0f;
        rect.size.y = content->rect.size.y + paddingY * 2.0f;
    }

    void UIMenuItem::layout(const Rect& rect)
    {
        this->rect = rect;
        if (!content)
        {
            return;
        }
        Vector2 pos(floorf(paddingX + 0.5f), floorf(paddingY + 0.5f));
        content->layout(Rect(pos, content->rect.size));
    }

    void UIMenuItem::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }

        primitive.pushScaleAround(rect.origin, localScale);
        Color bg = hovered ? hoverFill : background;
        primitive.addQuad(rect, UIFont::solidUV(), bg);
        primitive.popOrigin();
        UIWidget::render(primitive);
    }

    UIMenu::UIMenu()
    {
        list = std::make_shared<UIList>();
        list->direction = direction;
        list->padding = padding;
        list->spacing = spacing;
        list->color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        children.push_back(list);
    }

    void UIMenu::addItem(const std::shared_ptr<UIMenuItem>& item)
    {
        if (list && item)
        {
            list->addChild(item);
        }
    }

    void UIMenu::layout(const Rect& rect)
    {
        this->rect = rect;
        if (!list)
        {
            return;
        }

        list->direction = direction;
        list->padding = padding;
        list->spacing = spacing;

        float maxItemWidth = 0.0f;
        float maxItemHeight = 0.0f;
        float sumItemHeight = 0.0f;
        int itemCount = 0;
        for (auto& child : list->children)
        {
            UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
            if (item == nullptr || !item->visible)
            {
                continue;
            }
            item->syncSize();
            if (item->rect.size.x > maxItemWidth)
            {
                maxItemWidth = item->rect.size.x;
            }
            if (item->rect.size.y > maxItemHeight)
            {
                maxItemHeight = item->rect.size.y;
            }
            sumItemHeight += item->rect.size.y;
            itemCount += 1;
        }

        float gap = itemCount > 1 ? spacing * (float)(itemCount - 1) : 0.0f;
        if (direction == UIDirection::Horizontal)
        {
            this->rect.size.y = padding * 2.0f + maxItemHeight;
        }
        else
        {
            this->rect.size.x = padding * 2.0f + maxItemWidth;
            this->rect.size.y = padding * 2.0f + sumItemHeight + gap;
        }

        for (auto& child : list->children)
        {
            UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
            if (item == nullptr || !item->visible)
            {
                continue;
            }
            if (direction == UIDirection::Horizontal)
            {
                item->rect.size.y = maxItemHeight;
            }
            else
            {
                item->rect.size.x = maxItemWidth;
            }
        }

        list->layout(Rect(Vector2::ZERO, this->rect.size));
    }

    void UIMenu::render(UIPrimitive& primitive)
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
