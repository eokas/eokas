#include "UIMenu.h"
#include "UIFont.h"
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
            if (t->font != nullptr && t->font->isOpen())
            {
                float bake = (float)t->font->pixelSize();
                float scale = (t->fontSize > 0.0f ? t->fontSize : bake) / bake;
                float width = 0.0f;
                for (size_t i = 0; i < t->text.length(); i++)
                {
                    width += t->font->glyph(t->text.at(i)).advance * scale;
                }
                t->rect.width = floorf(width + 0.5f);
                float tight = t->font->ascender() - t->font->descender();
                t->rect.height = floorf(tight * scale + 0.5f);
            }
            rect.width = t->rect.width + paddingX * 2.0f;
            rect.height = t->rect.height + paddingY * 2.0f;
            return;
        }

        rect.width = content->rect.width + paddingX * 2.0f;
        rect.height = content->rect.height + paddingY * 2.0f;
    }

    void UIMenuItem::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }

        Color bg = hovered ? hoverColor : background;
        shape.addQuad(rect, UIFont::solidUV(), bg);

        if (content)
        {
            content->rect.x = floorf(rect.x + paddingX + 0.5f);
            content->rect.y = floorf(rect.y + paddingY + 0.5f);
        }
        UIWidget::render(shape);
    }

    UIMenu::UIMenu()
    {
        layout = std::make_shared<UILayout>();
        layout->direction = direction;
        layout->padding = padding;
        layout->spacing = spacing;
        layout->color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        children.push_back(layout);
    }

    void UIMenu::addItem(const std::shared_ptr<UIMenuItem>& item)
    {
        if (layout && item)
        {
            layout->addChild(item);
        }
    }

    void UIMenu::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }

        if (layout)
        {
            layout->direction = direction;
            layout->padding = padding;
            layout->spacing = spacing;

            float maxItemWidth = 0.0f;
            float maxItemHeight = 0.0f;
            float sumItemHeight = 0.0f;
            int itemCount = 0;
            for (auto& child : layout->children)
            {
                UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
                if (item == nullptr || !item->visible)
                {
                    continue;
                }
                item->syncSize();
                if (item->rect.width > maxItemWidth)
                {
                    maxItemWidth = item->rect.width;
                }
                if (item->rect.height > maxItemHeight)
                {
                    maxItemHeight = item->rect.height;
                }
                sumItemHeight += item->rect.height;
                itemCount += 1;
            }

            float gap = itemCount > 1 ? spacing * (float)(itemCount - 1) : 0.0f;
            if (direction == UILayoutDirection::Horizontal)
            {
                rect.height = padding * 2.0f + maxItemHeight;
            }
            else
            {
                rect.width = padding * 2.0f + maxItemWidth;
                rect.height = padding * 2.0f + sumItemHeight + gap;
            }

            for (auto& child : layout->children)
            {
                UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
                if (item == nullptr || !item->visible)
                {
                    continue;
                }
                if (direction == UILayoutDirection::Horizontal)
                {
                    item->rect.height = maxItemHeight;
                }
                else
                {
                    item->rect.width = maxItemWidth;
                }
            }

            layout->rect = rect;
        }

        if (color.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), color);
        }
        UIWidget::render(shape);
    }
}
