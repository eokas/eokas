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
        float rx = rect.size.x * 0.5f;
        float ry = rect.size.y * 0.5f;
        if (rx == 0.0f || ry == 0.0f)
        {
            return false;
        }
        float localX = point.x - rect.origin.x;
        float localY = point.y - rect.origin.y;
        float nx = (localX - rect.size.x * 0.5f) / rx;
        float ny = (localY - rect.size.y * 0.5f) / ry;
        return nx * nx + ny * ny <= 1.0f;
    }

    void UIEllipse::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        float rx = rect.size.x * 0.5f;
        float ry = rect.size.y * 0.5f;
        if (rx != 0.0f && ry != 0.0f)
        {
            Color fill = this->activeFill();
            int count = segmentCount(segments);
            Vector2 center = rect.origin + rect.size * 0.5f;
            std::vector<Vector2> boundary;
            boundary.reserve((size_t)count);
            std::vector<Vector2> vertices;
            vertices.reserve((size_t)count * 3);
            const float turn = 6.28318530718f;
            for (int i = 0; i < count; ++i)
            {
                float t = turn * (float)i / (float)count;
                Vector2 edge(rect.size.x * 0.5f + rx * cosf(t), rect.size.y * 0.5f + ry * sinf(t));
                boundary.push_back(edge);
            }
            for (int i = 0; i < count; ++i)
            {
                const Vector2& a = boundary[(size_t)i];
                const Vector2& b = boundary[(size_t)((i + 1) % count)];
                vertices.push_back(center);
                vertices.push_back(rect.origin + a);
                vertices.push_back(rect.origin + b);
            }
            primitive.addTriangles(vertices.data(), (uint32_t)count, UIFont::solidUV(), fill);
            this->strokeLoop(primitive, boundary);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
