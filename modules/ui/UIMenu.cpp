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
                t->rect.height = floorf(t->font->lineHeight() * scale + 0.5f);
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
        if (bg.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), bg);
        }

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
            layout->rect = rect;
            layout->direction = direction;
            layout->padding = padding;
            layout->spacing = spacing;
            for (auto& child : layout->children)
            {
                UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
                if (item != nullptr)
                {
                    item->syncSize();
                }
            }
        }

        if (color.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), color);
        }
        UIWidget::render(shape);
    }
}
