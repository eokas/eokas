#ifndef _EOKAS_UI_STROKE_H_
#define _EOKAS_UI_STROKE_H_

#include "UIShape.h"
#include "UIStyle.h"
#include <vector>

namespace eokas
{
    class UIPrimitive;

    class UIStroke
    {
    public:
        static void segment(UIPrimitive& primitive, const Matrix3& world, const Vector2& a, const Vector2& b, const UIStrokeStyle& style);
        static void path(UIPrimitive& primitive, const Matrix3& world, const std::vector<Vector2>& points, bool closed, const UIStrokeStyle& style);
        static void border(UIPrimitive& primitive, const Matrix3& world, const Rect& area, const UIStrokeStyle& style);
        static void marker(UIPrimitive& primitive, const Matrix3& world, const Vector2& tip, const Vector2& outward, const UIEndpointStyle& style);
    };
}

#endif//_EOKAS_UI_STROKE_H_
