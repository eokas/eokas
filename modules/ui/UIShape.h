#ifndef _EOKAS_UI_SHAPE_H_
#define _EOKAS_UI_SHAPE_H_

#include "base/main.h"

namespace eokas
{
    class UIShape
    {
    public:
        Vector2 origin { 0.0f, 0.0f };
        f32_t angle = 0.0f;
        Vector2 scale { 1.0f, 1.0f };
        Vector2 pivot { 0.5f, 0.5f };
        Vector2 size { 0.0f, 0.0f };

        Matrix3 localTrans() const;

        static Vector2 transformPoint(const Matrix3& matrix, const Vector2& point);
        static Vector2 transformVector(const Matrix3& matrix, const Vector2& vector);
        Vector2 toLocal(const Vector2& parentPivotPoint) const;
        Rect bounds(const Matrix3& toSpace) const;

        void setBox(const Vector2& topLeftInParentPivot, const Vector2& newSize);
        void setScaleAround(const Vector2& focalInParentPivot, const Vector2& nextScale);
    };
}

#endif//_EOKAS_UI_SHAPE_H_
