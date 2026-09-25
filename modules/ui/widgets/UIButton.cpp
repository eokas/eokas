#include "UIButton.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    UIButton::UIButton()
    {
        this->setContent(std::make_shared<UIText>());
    }

    void UIButton::setContent(const std::shared_ptr<UIWidget>& widget)
    {
        content = widget;
        children.clear();
        if (content)
        {
            children.push_back(content);
        }
    }

    void UIButton::setText(const String& text)
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

    UIText* UIButton::label() const
    {
        return dynamic_cast<UIText*>(content.get());
    }

    void UIButton::layout(const Rect& rect)
    {
        this->rect = rect;
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
        }

        float innerW = rect.width - paddingX * 2.0f;
        float innerH = rect.height - paddingY * 2.0f;
        if (innerW < 0.0f)
        {
            innerW = 0.0f;
        }
        if (innerH < 0.0f)
        {
            innerH = 0.0f;
        }
        float x = rect.x + paddingX + (innerW - content->rect.width) * 0.5f;
        float y = rect.y + paddingY + (innerH - content->rect.height) * 0.5f;
        content->layout(Rect(floorf(x + 0.5f), floorf(y + 0.5f), content->rect.width, content->rect.height));
    }

    void UIButton::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }

        Color bg = background;
        if (pressed)
        {
            bg = pressedColor;
        }
        else if (hovered)
        {
            bg = hoverColor;
        }
        shape.addQuad(rect, UIFont::solidUV(), bg);
        UIWidget::render(shape);
    }
}
