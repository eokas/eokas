#include "UIEllipse.h"
#include "../UIFont.h"

#include <cmath>
#include <vector>

namespace eokas
{
    namespace
    {
        int segmentCount(int segments)
        {
            return segments < 3 ? 3 : segments;
        }
    }

    UIEllipse::UIEllipse() = default;

    bool UIEllipse::contains(const Vector2& point) const
    {
        float rx = shape.size.x * 0.5f;
        float ry = shape.size.y * 0.5f;
        if (rx == 0.0f || ry == 0.0f)
        {
            return false;
        }
        float localX = point.x - shape.left();
        float localY = point.y - shape.top();
        float nx = (localX - shape.size.x * 0.5f) / rx;
        float ny = (localY - shape.size.y * 0.5f) / ry;
        return nx * nx + ny * ny <= 1.0f;
    }

    void UIEllipse::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(shape.origin, shape.scale);
        float rx = shape.size.x * 0.5f;
        float ry = shape.size.y * 0.5f;
        if (rx != 0.0f && ry != 0.0f)
        {
            Color fill = this->activeFill();
            int count = segmentCount(segments);
            Vector2 center = Vector2(shape.left(), shape.top()) + shape.size * 0.5f;
            std::vector<Vector2> boundary;
            boundary.reserve((size_t)count);
            std::vector<Vector2> vertices;
            vertices.reserve((size_t)count * 3);
            const float turn = 6.28318530718f;
            for (int i = 0; i < count; ++i)
            {
                float t = turn * (float)i / (float)count;
                Vector2 edge(shape.size.x * 0.5f + rx * cosf(t), shape.size.y * 0.5f + ry * sinf(t));
                boundary.push_back(edge);
            }
            for (int i = 0; i < count; ++i)
            {
                const Vector2& a = boundary[(size_t)i];
                const Vector2& b = boundary[(size_t)((i + 1) % count)];
                vertices.push_back(center);
                vertices.push_back(Vector2(shape.left(), shape.top()) + a);
                vertices.push_back(Vector2(shape.left(), shape.top()) + b);
            }
            primitive.addTriangles(vertices.data(), (uint32_t)count, UIFont::solidUV(), fill);
            this->strokeLoop(primitive, boundary);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
