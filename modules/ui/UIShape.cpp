#include "UIShape.h"

namespace eokas
{
    f32_t UIShape::left() const
    {
        return origin.x - pivot.x * size.x;
    }

    void UIShape::setLeft(f32_t value)
    {
        f32_t pinned = this->right();
        size.x = pinned - value;
        origin.x = value + pivot.x * size.x;
    }

    f32_t UIShape::right() const
    {
        return this->left() + size.x;
    }

    void UIShape::setRight(f32_t value)
    {
        f32_t edge = this->left();
        size.x = value - edge;
        origin.x = edge + pivot.x * size.x;
    }

    f32_t UIShape::top() const
    {
        return origin.y - pivot.y * size.y;
    }

    void UIShape::setTop(f32_t value)
    {
        f32_t pinned = this->bottom();
        size.y = pinned - value;
        origin.y = value + pivot.y * size.y;
    }

    f32_t UIShape::bottom() const
    {
        return this->top() + size.y;
    }

    void UIShape::setBottom(f32_t value)
    {
        f32_t edge = this->top();
        size.y = value - edge;
        origin.y = edge + pivot.y * size.y;
    }

    Rect UIShape::visualRect() const
    {
        Vector2 corner = origin - pivot * size;
        return Rect(this->toParent(corner), size * scale);
    }

    Vector2 UIShape::toParent(const Vector2& unscaled) const
    {
        return origin + (unscaled - origin) * scale;
    }

    Vector2 UIShape::toUnscaled(const Vector2& parentPoint) const
    {
        return Vector2(
            origin.x + (parentPoint.x - origin.x) / scale.x,
            origin.y + (parentPoint.y - origin.y) / scale.y);
    }

    Vector2 UIShape::toLocal(const Vector2& parentPoint) const
    {
        return this->toUnscaled(parentPoint) - (origin - pivot * size);
    }

    void UIShape::scaleAround(const Vector2& focal, const Vector2& nextScale)
    {
        auto axis = [](f32_t value) { return value == 0.0f ? 1.0f : value; };
        Vector2 oldScale(axis(scale.x), axis(scale.y));
        Vector2 fromPivot(
            (focal.x - origin.x) / oldScale.x,
            (focal.y - origin.y) / oldScale.y);
        origin = Vector2(
            focal.x - fromPivot.x * nextScale.x,
            focal.y - fromPivot.y * nextScale.y);
        scale = nextScale;
    }
}
