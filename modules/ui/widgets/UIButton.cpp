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

    void UIButton::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        if (content)
        {
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
                    float tight = font->ascender() - font->descender();
                    t->shape.size = Vector2(floorf(width + 0.5f), floorf(tight * scale + 0.5f));
                }
            }

            float innerW = shape.size.x - paddingX * 2.0f;
            float innerH = shape.size.y - paddingY * 2.0f;
            if (innerW < 0.0f)
            {
                innerW = 0.0f;
            }
            if (innerH < 0.0f)
            {
                innerH = 0.0f;
            }
            Vector2 pos(
                paddingX + (innerW - content->shape.size.x) * 0.5f,
                paddingY + (innerH - content->shape.size.y) * 0.5f);
            this->placeChild(*content, Vector2(floorf(pos.x + 0.5f), floorf(pos.y + 0.5f)));
        }

        Color bg = background;
        if (pressed)
        {
            bg = pressedFill;
        }
        else if (hovered)
        {
            bg = hoverFill;
        }
        primitive.addQuad(shape.worldTrans(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
        UIWidget::render(primitive);
    }
}
