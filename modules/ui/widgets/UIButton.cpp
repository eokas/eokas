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
        this->detachChildren();
        if (content)
        {
            this->attachChild(content);
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

    void UIButton::layout()
    {
        if (!visible)
        {
            return;
        }
        if (content)
        {
            content->layout();
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
    }

    void UIButton::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
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
        primitive.addQuad(matrixLocalToScreen(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
        UIWidget::render(primitive);
    }
}
