#include "UIToggle.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    namespace
    {
        constexpr int kDiscSegments = 16;

        float snap(float v)
        {
            return floorf(v + 0.5f);
        }
    }

    void UIToggle::setValue(bool v)
    {
        if (v == value)
        {
            return;
        }
        value = v;
        if (onValueChanged)
        {
            onValueChanged(value);
        }
    }

    void UIToggle::triggerClick()
    {
        this->setValue(!value);
        UIWidget::triggerClick();
    }

    Color UIToggle::capsuleColor() const
    {
        if (pressed)
        {
            return value ? onPressed : offPressed;
        }
        if (hovered)
        {
            return value ? onHover : offHover;
        }
        return value ? on : off;
    }

    void UIToggle::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }

        float thick = snap(Math::min_s(rect.size.x, rect.size.y));
        if (thick <= 0.0f)
        {
            UIWidget::render(primitive);
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);

        float radius = thick * 0.5f;
        float cy = snap(rect.origin.y + rect.size.y * 0.5f);
        float left = snap(rect.origin.x + radius);
        float right = snap(rect.origin.x + rect.size.x - radius);
        if (right < left)
        {
            right = left;
        }

        Color bg = this->capsuleColor();
        this->addDisc(primitive, left, cy, radius, bg);
        if (right > left)
        {
            primitive.addQuad(Rect(left, cy - radius, right - left, thick), UIFont::solidUV(), bg);
            this->addDisc(primitive, right, cy, radius, bg);
        }

        float inset = snap(thick * 0.12f);
        if (inset < 1.0f)
        {
            inset = 1.0f;
        }
        float thumbRadius = radius - inset;
        float thumbX = value ? right : left;
        this->addDisc(primitive, thumbX, cy, thumbRadius, thumb);
        primitive.popOrigin();
        UIWidget::render(primitive);
    }

    void UIToggle::addDisc(UIPrimitive& primitive, float cx, float cy, float radius, const Color& color)
    {
        if (radius <= 0.0f)
        {
            return;
        }
        Rect uv = UIFont::solidUV();
        Vector2 center(cx, cy);
        for (int i = 0; i < kDiscSegments; i++)
        {
            float a0 = Math::PI_MUL_2 * ((float)i / (float)kDiscSegments);
            float a1 = Math::PI_MUL_2 * ((float)(i + 1) / (float)kDiscSegments);
            Vector2 e0(cx + cosf(a0) * radius, cy + sinf(a0) * radius);
            Vector2 e1(cx + cosf(a1) * radius, cy + sinf(a1) * radius);
            primitive.addQuad(center, e0, e1, center, uv, color);
        }
    }
}
