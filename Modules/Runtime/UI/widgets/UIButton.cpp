#include "UIButton.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    UIButton::UIButton()
    {
        this->setContent(std::make_shared<UIText>());
    }

    void UIButton::setContent(const UIWidget::Ref& widget)
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
        if (isPressed())
        {
            bg = pressedFill;
        }
        else if (isHovered())
        {
            bg = hoverFill;
        }
        if (circle)
        {
            float radius = shape.size.x < shape.size.y ? shape.size.x : shape.size.y;
            radius *= 0.5f;
            float cx = shape.size.x * 0.5f;
            float cy = shape.size.y * 0.5f;
            Color rim(0.040f, 0.043f, 0.055f, 1.0f);
            this->addDisc(primitive, cx, cy, radius, rim);
            float inner = radius - 1.0f;
            if (inner > 0.0f)
            {
                this->addDisc(primitive, cx, cy, inner, bg);
            }
        }
        else
        {
            primitive.addQuad(matrixLocalToScreen(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
        }
        UIWidget::render(primitive);
    }

    bool UIButton::contains(const Vector2& point) const
    {
        if (!circle)
        {
            return UIWidget::contains(point);
        }
        float rx = shape.size.x * 0.5f;
        float ry = shape.size.y * 0.5f;
        if (rx <= 0.0f || ry <= 0.0f)
        {
            return false;
        }
        float dx = (point.x - rx) / rx;
        float dy = (point.y - ry) / ry;
        return dx * dx + dy * dy <= 1.0f;
    }

    void UIButton::addDisc(UIPrimitive& primitive, float cx, float cy, float radius, const Color& color)
    {
        if (radius <= 0.0f || color.a <= 0.0f)
        {
            return;
        }
        constexpr int kDiscSegments = 24;
        Rect uv = UIFont::solidUV();
        Vector2 center(cx, cy);
        for (int i = 0; i < kDiscSegments; i++)
        {
            float a0 = Math::PI_MUL_2 * ((float)i / (float)kDiscSegments);
            float a1 = Math::PI_MUL_2 * ((float)(i + 1) / (float)kDiscSegments);
            Vector2 e0(cx + cosf(a0) * radius, cy + sinf(a0) * radius);
            Vector2 e1(cx + cosf(a1) * radius, cy + sinf(a1) * radius);
            primitive.addQuad(
                this->localToScreen(center),
                this->localToScreen(e0),
                this->localToScreen(e1),
                this->localToScreen(center),
                uv,
                color);
        }
    }
}
