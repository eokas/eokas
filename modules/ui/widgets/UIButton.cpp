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
        }

        float innerW = rect.size.x - paddingX * 2.0f;
        float innerH = rect.size.y - paddingY * 2.0f;
        if (innerW < 0.0f)
        {
            innerW = 0.0f;
        }
        if (innerH < 0.0f)
        {
            innerH = 0.0f;
        }
        Vector2 pos(
            paddingX + (innerW - content->rect.size.x) * 0.5f,
            paddingY + (innerH - content->rect.size.y) * 0.5f);
        content->layout(Rect(Vector2(floorf(pos.x + 0.5f), floorf(pos.y + 0.5f)), content->rect.size));
    }

    void UIButton::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }

        primitive.pushScaleAround(rect.origin, localScale);
        Color bg = background;
        if (pressed)
        {
            bg = pressedFill;
        }
        else if (hovered)
        {
            bg = hoverFill;
        }
        primitive.addQuad(rect, UIFont::solidUV(), bg);
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
