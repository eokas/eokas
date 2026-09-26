#ifndef _EOKAS_UI_SHAPE_H_
#define _EOKAS_UI_SHAPE_H_

#include "base/main.h"

namespace eokas
{
    // origin is the pivot position in parent space.
    // pivot is normalized on size: (0, 0) top-left, (1, 1) bottom-right.
    // Default pivot is the center, (0.5, 0.5).
    // Unscaled top-left is origin - pivot * size.
    // Child coordinates stay relative to that top-left.
    // left/right/top/bottom are unscaled layout edges and do not include scale.
    class UIShape
    {
    public:
        Vector2 origin { 0.0f, 0.0f };
        Vector2 size { 0.0f, 0.0f };
        Vector2 pivot { 0.5f, 0.5f };
        Vector2 scale { 1.0f, 1.0f };

        f32_t left() const;
        void setLeft(f32_t value);
        f32_t right() const;
        void setRight(f32_t value);
        f32_t top() const;
        void setTop(f32_t value);
        f32_t bottom() const;
        void setBottom(f32_t value);

        Rect visualRect() const;
        Vector2 toLocal(const Vector2& parentPoint) const;
        Vector2 toUnscaled(const Vector2& parentPoint) const;
        Vector2 toParent(const Vector2& unscaled) const;
        void scaleAround(const Vector2& focal, const Vector2& nextScale);
    };
}

#endif//_EOKAS_UI_SHAPE_H_
