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

    bool UISlider::handlePointerRelease(float screenX, float screenY, int button)
    {
        mTracking = false;
        return UIWidget::handlePointerRelease(screenX, screenY, button);
    }

    bool UISlider::handlePointerMove(float screenX, float screenY, const Vector2& delta)
    {
        Vector2 position = parent() != nullptr ? parent()->screenToPivot(Vector2(screenX, screenY)) : Vector2(screenX, screenY);
        bool handled = UIWidget::handlePointerMove(screenX, screenY, delta);
        if (!isPressed() || !pickable)
        {
            return handled;
        }
        if (maxValue <= minValue)
        {
            this->commitValue(minValue);
            return true;
        }

        bool begin = !mTracking;
        mTracking = true;

        float t = 0.0f;
        if (this->ring())
        {
            t = this->ringT(position.x, position.y, begin);
        }
        else if (type == SliderType::Vertical)
        {
            float half = thumbSize * 0.5f;
            float span = shape.size.y - thumbSize;
            Vector2 corner = shape.origin - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
            float origin = corner.y + shape.size.y - half;
            t = (span > 0.0f) ? Math::clamp((origin - position.y) / span, 0.0f, 1.0f) : 0.0f;
        }
        else
        {
            float half = thumbSize * 0.5f;
            float span = shape.size.x - thumbSize;
            Vector2 corner = shape.origin - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
            float origin = corner.x + half;
            t = (span > 0.0f) ? Math::clamp((position.x - origin) / span, 0.0f, 1.0f) : 0.0f;
        }
        this->commitValue(minValue + (maxValue - minValue) * t);
        return true;
    }

    bool UISlider::ring() const
    {
        return type == SliderType::Clock || type == SliderType::Angular;
    }

    float UISlider::pointerAngle(float x, float y) const
    {
        Vector2 corner = shape.origin - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
        float cx = corner.x + shape.size.x * 0.5f;
        float cy = corner.y + shape.size.y * 0.5f;
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
        if (!pickable)
        {
            return thumb;
        }
        if (isPressed())
        {
            return thumbPressed;
        }
        if (isHovered())
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
        Matrix3 localToScreen = matrixLocalToScreen();
        if (this->ring())
        {
            this->renderRing(primitive, localToScreen);
        }
        else
        {
            this->renderLinear(primitive, localToScreen, type == SliderType::Vertical);
        }
        UIWidget::render(primitive);
    }

    void UISlider::renderLinear(UIPrimitive& primitive, const Matrix3& localToScreen, bool vertical)
    {
        float t = this->valueT();
        float half = thumbSize * 0.5f;
        Rect solid = UIFont::solidUV();
        if (vertical)
        {
            float thick = trackThickness;
            if (thick > shape.size.x)
            {
                thick = shape.size.x;
            }
            float axisX = snap(shape.size.x * 0.5f);
            float trackX = axisX - thick * 0.5f;
            primitive.addQuad(localToScreen, Rect(trackX, 0.0f, thick, shape.size.y), solid, track);

            float span = shape.size.y - thumbSize;
            if (span < 0.0f)
            {
                span = 0.0f;
            }
            float thumbCenter = snap(shape.size.y - half - span * t);
            float fillH = shape.size.y - thumbCenter;
            if (fillH > 0.0f)
            {
                primitive.addQuad(localToScreen, Rect(trackX, thumbCenter, thick, fillH), solid, fill);
            }
            this->addDisc(primitive, localToScreen, axisX, thumbCenter, half, this->thumbDrawColor());
            return;
        }

        float thick = trackThickness;
        if (thick > shape.size.y)
        {
            thick = shape.size.y;
        }
        float axisY = snap(shape.size.y * 0.5f);
        float trackY = axisY - thick * 0.5f;
        primitive.addQuad(localToScreen, Rect(0.0f, trackY, shape.size.x, thick), solid, track);

        float span = shape.size.x - thumbSize;
        if (span < 0.0f)
        {
            span = 0.0f;
        }
        float thumbCenter = snap(half + span * t);
        if (thumbCenter > 0.0f)
        {
            primitive.addQuad(localToScreen, Rect(0.0f, trackY, thumbCenter, thick), solid, fill);
        }
        this->addDisc(primitive, localToScreen, thumbCenter, axisY, half, this->thumbDrawColor());
    }

    void UISlider::renderRing(UIPrimitive& primitive, const Matrix3& localToScreen)
    {
        float t = this->valueT();
        float cx = snap(shape.size.x * 0.5f);
        float cy = snap(shape.size.y * 0.5f);
        float extent = Math::min_s(shape.size.x, shape.size.y);
        float inset = Math::max_s(thumbSize, trackThickness) * 0.5f;
        float radius = extent * 0.5f - inset;
        if (radius > 0.0f)
        {
            this->addArc(primitive, localToScreen, cx, cy, radius, 0.0f, Math::PI_MUL_2, track);
            if (t >= 1.0f)
            {
                this->addArc(primitive, localToScreen, cx, cy, radius, 0.0f, Math::PI_MUL_2, fill);
            }
            else if (t > 0.0f)
            {
                this->addArc(primitive, localToScreen, cx, cy, radius, 0.0f, t * Math::PI_MUL_2, fill);
            }
        }

        float drawRadius = radius > 0.0f ? radius : 0.0f;
        Vector2 p = this->ringPoint(cx, cy, drawRadius, t * Math::PI_MUL_2);
        this->addDisc(primitive, localToScreen, p.x, p.y, thumbSize * 0.5f, this->thumbDrawColor());
    }

    void UISlider::addDisc(UIPrimitive& primitive, const Matrix3& localToScreen, float cx, float cy, float radius, const Color& color)
    {
        (void)localToScreen;
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
            primitive.addQuad(
                this->localToScreen(center),
                this->localToScreen(e0),
                this->localToScreen(e1),
                this->localToScreen(center),
                uv,
                color);
        }
    }

    void UISlider::addArc(UIPrimitive& primitive, const Matrix3& localToScreen, float cx, float cy, float radius, float a0, float a1, const Color& color)
    {
        (void)localToScreen;
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
                this->localToScreen(this->ringPoint(cx, cy, inner, s0)),
                this->localToScreen(this->ringPoint(cx, cy, outer, s0)),
                this->localToScreen(this->ringPoint(cx, cy, outer, s1)),
                this->localToScreen(this->ringPoint(cx, cy, inner, s1)),
                uv,
                color);
        }
    }
}
