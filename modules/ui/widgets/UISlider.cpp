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
            float span = rect.size.y - thumbSize;
            float origin = rect.origin.y + rect.size.y - half;
            t = (span > 0.0f) ? Math::clamp((origin - y) / span, 0.0f, 1.0f) : 0.0f;
        }
        else
        {
            float half = thumbSize * 0.5f;
            float span = rect.size.x - thumbSize;
            float origin = rect.origin.x + half;
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
        float cx = rect.origin.x + rect.size.x * 0.5f;
        float cy = rect.origin.y + rect.size.y * 0.5f;
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
            return thumb;
        }
        if (pressed)
        {
            return thumbPressed;
        }
        if (hovered)
        {
            return thumbHover;
        }
        return thumb;
    }

    void UISlider::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        if (this->ring())
        {
            this->renderRing(primitive);
        }
        else
        {
            this->renderLinear(primitive, type == SliderType::Vertical);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }

    void UISlider::renderLinear(UIPrimitive& primitive, bool vertical)
    {
        float t = this->valueT();
        float half = thumbSize * 0.5f;
        Rect solid = UIFont::solidUV();
        if (vertical)
        {
            float thick = trackThickness;
            if (thick > rect.size.x)
            {
                thick = rect.size.x;
            }
            float axisX = snap(rect.origin.x + rect.size.x * 0.5f);
            float trackX = axisX - thick * 0.5f;
            primitive.addQuad(Rect(trackX, rect.origin.y, thick, rect.size.y), solid, track);

            float span = rect.size.y - thumbSize;
            if (span < 0.0f)
            {
                span = 0.0f;
            }
            float thumbCenter = snap(rect.origin.y + rect.size.y - half - span * t);
            float fillH = rect.origin.y + rect.size.y - thumbCenter;
            if (fillH > 0.0f)
            {
                primitive.addQuad(Rect(trackX, thumbCenter, thick, fillH), solid, fill);
            }
            this->addDisc(primitive, axisX, thumbCenter, half, this->thumbDrawColor());
            return;
        }

        float thick = trackThickness;
        if (thick > rect.size.y)
        {
            thick = rect.size.y;
        }
        float axisY = snap(rect.origin.y + rect.size.y * 0.5f);
        float trackY = axisY - thick * 0.5f;
        primitive.addQuad(Rect(rect.origin.x, trackY, rect.size.x, thick), solid, track);

        float span = rect.size.x - thumbSize;
        if (span < 0.0f)
        {
            span = 0.0f;
        }
        float thumbCenter = snap(rect.origin.x + half + span * t);
        if (thumbCenter > rect.origin.x)
        {
            primitive.addQuad(Rect(rect.origin.x, trackY, thumbCenter - rect.origin.x, thick), solid, fill);
        }
        this->addDisc(primitive, thumbCenter, axisY, half, this->thumbDrawColor());
    }

    void UISlider::renderRing(UIPrimitive& primitive)
    {
        float t = this->valueT();
        float cx = snap(rect.origin.x + rect.size.x * 0.5f);
        float cy = snap(rect.origin.y + rect.size.y * 0.5f);
        float extent = Math::min_s(rect.size.x, rect.size.y);
        float inset = Math::max_s(thumbSize, trackThickness) * 0.5f;
        float radius = extent * 0.5f - inset;
        if (radius > 0.0f)
        {
            this->addArc(primitive, cx, cy, radius, 0.0f, Math::PI_MUL_2, track);
            if (t >= 1.0f)
            {
                this->addArc(primitive, cx, cy, radius, 0.0f, Math::PI_MUL_2, fill);
            }
            else if (t > 0.0f)
            {
                this->addArc(primitive, cx, cy, radius, 0.0f, t * Math::PI_MUL_2, fill);
            }
        }

        float drawRadius = radius > 0.0f ? radius : 0.0f;
        Vector2 p = this->ringPoint(cx, cy, drawRadius, t * Math::PI_MUL_2);
        this->addDisc(primitive, p.x, p.y, thumbSize * 0.5f, this->thumbDrawColor());
    }

    void UISlider::addDisc(UIPrimitive& primitive, float cx, float cy, float radius, const Color& color)
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
            primitive.addQuad(center, e0, e1, center, uv, color);
        }
    }

    void UISlider::addArc(UIPrimitive& primitive, float cx, float cy, float radius, float a0, float a1, const Color& color)
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
            primitive.addQuad(
                this->ringPoint(cx, cy, inner, s0),
                this->ringPoint(cx, cy, outer, s0),
                this->ringPoint(cx, cy, outer, s1),
                this->ringPoint(cx, cy, inner, s1),
                uv,
                color);
        }
    }
}
