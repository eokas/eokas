#include "UIDiamond.h"
#include "../UIFont.h"
#include "../UIStroke.h"

#include <vector>

namespace eokas
{
    namespace
    {
        void diamondPoints(const Rect& area, float pad, std::vector<Vector2>& out)
        {
            Vector2 top(area.origin.x + area.size.x * 0.5f, area.origin.y - pad);
            Vector2 right(area.origin.x + area.size.x + pad, area.origin.y + area.size.y * 0.5f);
            Vector2 bottom(area.origin.x + area.size.x * 0.5f, area.origin.y + area.size.y + pad);
            Vector2 left(area.origin.x - pad, area.origin.y + area.size.y * 0.5f);
            out.push_back(top);
            out.push_back(right);
            out.push_back(bottom);
            out.push_back(left);
        }
    }

    UIDiamond::UIDiamond()
    {
        border.thickness = 2.0f;
    }

    bool UIDiamond::contains(const Vector2& point) const
    {
        if (rect.size.x <= 0.0f || rect.size.y <= 0.0f)
        {
            return false;
        }
        Vector2 local(point.x - rect.origin.x, point.y - rect.origin.y);
        Vector2 vertex[4] = {
            Vector2(rect.size.x * 0.5f, 0.0f),
            Vector2(rect.size.x, rect.size.y * 0.5f),
            Vector2(rect.size.x * 0.5f, rect.size.y),
            Vector2(0.0f, rect.size.y * 0.5f)
        };
        float sign = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            const Vector2& a = vertex[i];
            const Vector2& b = vertex[(i + 1) % 4];
            float cross = (b.x - a.x) * (local.y - a.y) - (b.y - a.y) * (local.x - a.x);
            if (cross == 0.0f)
            {
                continue;
            }
            if (sign == 0.0f)
            {
                sign = cross;
            }
            else if ((sign > 0.0f) != (cross > 0.0f))
            {
                return false;
            }
        }
        return true;
    }

    void UIDiamond::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        if (rect.size.x > 0.0f && rect.size.y > 0.0f)
        {
            std::vector<Vector2> loop;
            diamondPoints(rect, 0.0f, loop);
            Vector2 vertices[6] = {
                loop[0], loop[1], loop[2],
                loop[0], loop[2], loop[3]
            };
            primitive.addTriangles(vertices, 2, UIFont::solidUV(), this->activeFill());
            if (border.thickness > 0.0f)
            {
                UIStroke::path(primitive, loop, true, border);
            }
            if (selected && stroke.thickness > 0.0f)
            {
                float pad = (border.thickness + stroke.thickness) * 0.5f;
                std::vector<Vector2> outer;
                diamondPoints(rect, pad, outer);
                UIStroke::path(primitive, outer, true, stroke);
            }
        }
        primitive.popOrigin();
        this->placeLabel();
        UIWidget::render(primitive);
    }
}
