#include "UIShape.h"

#include <cmath>

namespace eokas
{
    UIShape::UIShape(const UIShape& other)
        : parent(nullptr)
        , origin(other.origin)
        , angle(other.angle)
        , scale(other.scale)
        , pivot(other.pivot)
        , size(other.size)
    {
    }

    UIShape& UIShape::operator=(const UIShape& other)
    {
        if (this != &other)
        {
            parent = nullptr;
            origin = other.origin;
            angle = other.angle;
            scale = other.scale;
            pivot = other.pivot;
            size = other.size;
        }
        return *this;
    }

    Matrix3 UIShape::localTrans() const
    {
        Matrix3 m = Matrix3::translation(Vector2(-pivot.x * size.x, -pivot.y * size.y));
        m = Matrix3::transform(m, Matrix3::scaling(scale));
        m = Matrix3::transform(m, Matrix3::rotation(Vector2::ZERO, angle));
        m = Matrix3::transform(m, Matrix3::translation(origin));
        return m;
    }

    Matrix3 UIShape::pivotToScreen() const
    {
        return Matrix3::transform(Matrix3::translation(Vector2(pivot.x * size.x, pivot.y * size.y)), this->worldTrans());
    }

    Matrix3 UIShape::worldTrans() const
    {
        Matrix3 local = this->localTrans();
        if (parent == nullptr)
        {
            return local;
        }
        return Matrix3::transform(local, parent->pivotToScreen());
    }

    Vector2 UIShape::transformPoint(const Matrix3& matrix, const Vector2& point)
    {
        Vector3 p = Matrix3::transform(Vector3(point.x, point.y, 1.0f), matrix);
        return Vector2(p.x, p.y);
    }

    Vector2 UIShape::transformVector(const Matrix3& matrix, const Vector2& vector)
    {
        Vector3 p = Matrix3::transform(Vector3(vector.x, vector.y, 0.0f), matrix);
        return Vector2(p.x, p.y);
    }

    Vector2 UIShape::toLocal(const Vector2& parentPivotPoint) const
    {
        return transformPoint(this->localTrans().inverse(), parentPivotPoint);
    }

    Rect UIShape::bounds(const Matrix3& toSpace) const
    {
        Vector2 corner[4] = {
            transformPoint(toSpace, Vector2(0.0f, 0.0f)),
            transformPoint(toSpace, Vector2(size.x, 0.0f)),
            transformPoint(toSpace, Vector2(size.x, size.y)),
            transformPoint(toSpace, Vector2(0.0f, size.y))
        };
        float minX = corner[0].x;
        float minY = corner[0].y;
        float maxX = corner[0].x;
        float maxY = corner[0].y;
        for (int i = 1; i < 4; ++i)
        {
            minX = corner[i].x < minX ? corner[i].x : minX;
            minY = corner[i].y < minY ? corner[i].y : minY;
            maxX = corner[i].x > maxX ? corner[i].x : maxX;
            maxY = corner[i].y > maxY ? corner[i].y : maxY;
        }
        return Rect(minX, minY, maxX - minX, maxY - minY);
    }

    void UIShape::setBox(const Vector2& topLeftInParentPivot, const Vector2& newSize)
    {
        size = newSize;
        origin = topLeftInParentPivot + Vector2(pivot.x * newSize.x, pivot.y * newSize.y);
    }

    void UIShape::setScaleAround(const Vector2& focal, const Vector2& nextScale)
    {
        auto axis = [](f32_t value) { return value == 0.0f ? 1.0f : value; };
        Vector2 oldScale(axis(scale.x), axis(scale.y));
        float c = cosf(angle);
        float s = sinf(angle);
        Vector2 delta = focal - origin;
        Vector2 unrotated(delta.x * c + delta.y * s, -delta.x * s + delta.y * c);
        Vector2 v(unrotated.x / oldScale.x, unrotated.y / oldScale.y);
        Vector2 scaled(v.x * nextScale.x, v.y * nextScale.y);
        Vector2 rotated(scaled.x * c - scaled.y * s, scaled.x * s + scaled.y * c);
        origin = focal - rotated;
        scale = nextScale;
    }
}
