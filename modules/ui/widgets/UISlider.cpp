#include "UISlider.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    namespace
    {
        constexpr int kRingSegments = 48;
        constexpr int kThumbSegments = 16;
        constexpr float kSeam = 0.02f;

        float snap(float v)
        {
            return floorf(v + 0.5f);
        }
    }

    void UISlider::setValue(float v)
    {
        this->commitValue(v);
    }

    void UISlider::setRange(float minV, float maxV)
    {
        if (minV > maxV)
        {
            float tmp = minV;
            minV = maxV;
            maxV = tmp;
        }
        minValue = minV;
        maxValue = maxV;
        this->commitValue(value);
    }

    void UISlider::commitValue(float next)
    {
        if (next < minValue)
        {
            next = minValue;
        }
        if (next > maxValue)
        {
            next = maxValue;
        }
        if (next == value)
        {
            return;
        }
        value = next;
        if (onValueChanged)
        {
            onValueChanged(value);
        }
    }

    float UISlider::valueT() const
    {
        float span = maxValue - minValue;
        if (span <= 0.0f)
        {
            return 0.0f;
        }
        return (value - minValue) / span;
    }

    void UISlider::triggerPointerRelease()
    {
        mTracking = false;
        UIWidget::triggerPointerRelease();
    }

    void UISlider::resetPointerState()
    {
        mTracking = false;
        UIWidget::resetPointerState();
    }

    void UISlider::triggerPointerDrag(float x, float y, int button)
    {
        if (button != 0 || !interactive)
        {
            return;
        }
        if (maxValue <= minValue)
        {
            this->commitValue(minValue);
            return;
        }

        bool begin = !mTracking;
        mTracking = true;

        float t = 0.0f;
        if (this->ring())
        {
            t = this->ringT(x, y, begin);
        }
        else if (type == SliderType::Vertical)
        {
            float half = thumbSize * 0.5f;
            float span = rect.height - thumbSize;
            float origin = rect.y + rect.height - half;
            t = (span > 0.0f) ? Math::clamp((origin - y) / span, 0.0f, 1.0f) : 0.0f;
        }
        else
        {
            float half = thumbSize * 0.5f;
            float span = rect.width - thumbSize;
            float origin = rect.x + half;
            t = (span > 0.0f) ? Math::clamp((x - origin) / span, 0.0f, 1.0f) : 0.0f;
        }
        this->commitValue(minValue + (maxValue - minValue) * t);
    }

    bool UISlider::ring() const
    {
        return type == SliderType::Clock || type == SliderType::Angular;
    }

    float UISlider::pointerAngle(float x, float y) const
    {
        float cx = rect.x + rect.width * 0.5f;
        float cy = rect.y + rect.height * 0.5f;
        float dx = x - cx;
        float dy = y - cy;
        float angle = (type == SliderType::Angular) ? atan2f(-dy, dx) : atan2f(dx, -dy);
        if (angle < 0.0f)
        {
            angle += Math::PI_MUL_2;
        }
        return angle;
    }

    Vector2 UISlider::ringPoint(float cx, float cy, float radius, float angle) const
    {
        if (type == SliderType::Angular)
        {
            return Vector2(cx + cosf(angle) * radius, cy - sinf(angle) * radius);
        }
        return Vector2(cx + sinf(angle) * radius, cy - cosf(angle) * radius);
    }

    float UISlider::ringT(float x, float y, bool begin)
    {
        float angle = this->pointerAngle(x, y);

        if (begin)
        {
            if (angle <= kSeam || angle >= Math::PI_MUL_2 - kSeam)
            {
                float mid = (minValue + maxValue) * 0.5f;
                mAccum = (value >= mid) ? Math::PI_MUL_2 : 0.0f;
            }
            else
            {
                mAccum = angle;
            }
            mLastAngle = angle;
        }
        else
        {
            float delta = angle - mLastAngle;
            if (delta > Math::PI)
            {
                delta -= Math::PI_MUL_2;
            }
            if (delta < -Math::PI)
            {
                delta += Math::PI_MUL_2;
            }
            mAccum += delta;
            if (mAccum < 0.0f)
            {
                mAccum = 0.0f;
            }
            if (mAccum > Math::PI_MUL_2)
            {
                mAccum = Math::PI_MUL_2;
            }
            mLastAngle = (mAccum <= 0.0f || mAccum >= Math::PI_MUL_2) ? 0.0f : angle;
        }

        return mAccum / Math::PI_MUL_2;
    }

    Color UISlider::thumbDrawColor() const
    {
        if (!interactive)
        {
            return thumbColor;
        }
        if (pressed)
        {
            return thumbPressedColor;
        }
        if (hovered)
        {
            return thumbHoverColor;
        }
        return thumbColor;
    }

    void UISlider::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        if (this->ring())
        {
            this->renderRing(shape);
        }
        else
        {
            this->renderLinear(shape, type == SliderType::Vertical);
        }
        UIWidget::render(shape);
    }

    void UISlider::renderLinear(UIShape& shape, bool vertical)
    {
        float t = this->valueT();
        float half = thumbSize * 0.5f;
        Rect solid = UIFont::solidUV();
        if (vertical)
        {
            float thick = trackThickness;
            if (thick > rect.width)
            {
                thick = rect.width;
            }
            float axisX = snap(rect.x + rect.width * 0.5f);
            float trackX = axisX - thick * 0.5f;
            shape.addQuad(Rect(trackX, rect.y, thick, rect.height), solid, trackColor);

            float span = rect.height - thumbSize;
            if (span < 0.0f)
            {
                span = 0.0f;
            }
            float thumbCenter = snap(rect.y + rect.height - half - span * t);
            float fillH = rect.y + rect.height - thumbCenter;
            if (fillH > 0.0f)
            {
                shape.addQuad(Rect(trackX, thumbCenter, thick, fillH), solid, fillColor);
            }
            this->addDisc(shape, axisX, thumbCenter, half, this->thumbDrawColor());
            return;
        }

        float thick = trackThickness;
        if (thick > rect.height)
        {
            thick = rect.height;
        }
        float axisY = snap(rect.y + rect.height * 0.5f);
        float trackY = axisY - thick * 0.5f;
        shape.addQuad(Rect(rect.x, trackY, rect.width, thick), solid, trackColor);

        float span = rect.width - thumbSize;
        if (span < 0.0f)
        {
            span = 0.0f;
        }
        float thumbCenter = snap(rect.x + half + span * t);
        if (thumbCenter > rect.x)
        {
            shape.addQuad(Rect(rect.x, trackY, thumbCenter - rect.x, thick), solid, fillColor);
        }
        this->addDisc(shape, thumbCenter, axisY, half, this->thumbDrawColor());
    }

    void UISlider::renderRing(UIShape& shape)
    {
        float t = this->valueT();
        float cx = snap(rect.x + rect.width * 0.5f);
        float cy = snap(rect.y + rect.height * 0.5f);
        float extent = Math::min_s(rect.width, rect.height);
        float inset = Math::max_s(thumbSize, trackThickness) * 0.5f;
        float radius = extent * 0.5f - inset;
        if (radius > 0.0f)
        {
            this->addArc(shape, cx, cy, radius, 0.0f, Math::PI_MUL_2, trackColor);
            if (t >= 1.0f)
            {
                this->addArc(shape, cx, cy, radius, 0.0f, Math::PI_MUL_2, fillColor);
            }
            else if (t > 0.0f)
            {
                this->addArc(shape, cx, cy, radius, 0.0f, t * Math::PI_MUL_2, fillColor);
            }
        }

        float drawRadius = radius > 0.0f ? radius : 0.0f;
        Vector2 p = this->ringPoint(cx, cy, drawRadius, t * Math::PI_MUL_2);
        this->addDisc(shape, p.x, p.y, thumbSize * 0.5f, this->thumbDrawColor());
    }

    void UISlider::addDisc(UIShape& shape, float cx, float cy, float radius, const Color& color)
    {
        if (radius <= 0.0f)
        {
            return;
        }
        Rect uv = UIFont::solidUV();
        Vector2 center(cx, cy);
        for (int i = 0; i < kThumbSegments; i++)
        {
            float a0 = Math::PI_MUL_2 * ((float)i / (float)kThumbSegments);
            float a1 = Math::PI_MUL_2 * ((float)(i + 1) / (float)kThumbSegments);
            Vector2 e0(cx + cosf(a0) * radius, cy + sinf(a0) * radius);
            Vector2 e1(cx + cosf(a1) * radius, cy + sinf(a1) * radius);
            shape.addQuad(center, e0, e1, center, uv, color);
        }
    }

    void UISlider::addArc(UIShape& shape, float cx, float cy, float radius, float a0, float a1, const Color& color)
    {
        float sweep = a1 - a0;
        if (sweep <= 0.0f)
        {
            return;
        }
        float inner = radius - trackThickness * 0.5f;
        float outer = radius + trackThickness * 0.5f;
        if (inner < 0.0f)
        {
            inner = 0.0f;
        }
        int steps = (int)ceilf(sweep / Math::PI_MUL_2 * (float)kRingSegments);
        if (steps < 1)
        {
            steps = 1;
        }
        Rect uv = UIFont::solidUV();
        for (int i = 0; i < steps; i++)
        {
            float s0 = a0 + sweep * ((float)i / (float)steps);
            float s1 = a0 + sweep * ((float)(i + 1) / (float)steps);
            shape.addQuad(
                this->ringPoint(cx, cy, inner, s0),
                this->ringPoint(cx, cy, outer, s0),
                this->ringPoint(cx, cy, outer, s1),
                this->ringPoint(cx, cy, inner, s1),
                uv,
                color);
        }
    }
}
