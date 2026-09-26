#include "UIStroke.h"
#include "UIFont.h"
#include "UIPrimitive.h"

#include <cmath>
#include <vector>

namespace eokas
{
    namespace
    {
        constexpr int kMarkerSegments = 24;

        UIStrokeStyle solidOf(const UIStrokeStyle& style)
        {
            UIStrokeStyle solid = style;
            solid.pattern = UILinePattern::Solid;
            return solid;
        }

        void addDisc(UIPrimitive& primitive, const Vector2& center, float radius, const Color& color)
        {
            if (radius <= 0.0f || color.a <= 0.0f)
            {
                return;
            }
            Rect uv = UIFont::solidUV();
            std::vector<Vector2> vertices;
            vertices.reserve((size_t)kMarkerSegments * 3);
            for (int i = 0; i < kMarkerSegments; ++i)
            {
                float a0 = 6.28318530718f * (float)i / (float)kMarkerSegments;
                float a1 = 6.28318530718f * (float)(i + 1) / (float)kMarkerSegments;
                vertices.push_back(center);
                vertices.push_back(Vector2(center.x + cosf(a0) * radius, center.y + sinf(a0) * radius));
                vertices.push_back(Vector2(center.x + cosf(a1) * radius, center.y + sinf(a1) * radius));
            }
            primitive.addTriangles(vertices.data(), (uint32_t)kMarkerSegments, uv, color);
        }

        void addRing(UIPrimitive& primitive, const Vector2& center, float radius, float thickness, const Color& color)
        {
            if (radius <= 0.0f || thickness <= 0.0f || color.a <= 0.0f)
            {
                return;
            }
            float inner = radius - thickness;
            if (inner <= 0.0f)
            {
                addDisc(primitive, center, radius, color);
                return;
            }
            Rect uv = UIFont::solidUV();
            for (int i = 0; i < kMarkerSegments; ++i)
            {
                float a0 = 6.28318530718f * (float)i / (float)kMarkerSegments;
                float a1 = 6.28318530718f * (float)(i + 1) / (float)kMarkerSegments;
                Vector2 d0(cosf(a0), sinf(a0));
                Vector2 d1(cosf(a1), sinf(a1));
                primitive.addQuad(center + d0 * radius, center + d1 * radius, center + d1 * inner, center + d0 * inner, uv, color);
            }
        }

        bool nearPoint(const Vector2& a, const Vector2& b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            return dx * dx + dy * dy <= 1.0e-8f;
        }

        void addJoinTriangle(UIPrimitive& primitive, const Vector2& a, const Vector2& b, const Vector2& c, const Rect& uv, const Color& color)
        {
            float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            if (cross == 0.0f)
            {
                return;
            }
            Vector2 tri[3];
            tri[0] = a;
            if (cross > 0.0f)
            {
                tri[1] = b;
                tri[2] = c;
            }
            else
            {
                tri[1] = c;
                tri[2] = b;
            }
            primitive.addTriangles(tri, 1, uv, color);
        }

        void addCorner(UIPrimitive& primitive, const Vector2& vertex, const Vector2& dirIn, const Vector2& dirOut, float half, const Color& color)
        {
            float cross = dirIn.x * dirOut.y - dirIn.y * dirOut.x;
            float dot = dirIn.x * dirOut.x + dirIn.y * dirOut.y;
            if (fabsf(cross) < 1.0e-4f && dot > 0.0f)
            {
                return;
            }
            Vector2 n0(-dirIn.y, dirIn.x);
            Vector2 n1(-dirOut.y, dirOut.x);
            Rect uv = UIFont::solidUV();
            if (dot < -0.999f)
            {
                addJoinTriangle(primitive, vertex, vertex + n0 * half, vertex + n1 * half, uv, color);
                addJoinTriangle(primitive, vertex, vertex - n0 * half, vertex - n1 * half, uv, color);
                return;
            }
            float side = cross > 0.0f ? -1.0f : 1.0f;
            Vector2 outer0 = vertex + n0 * (half * side);
            Vector2 outer1 = vertex + n1 * (half * side);
            Vector2 dm = (n0 + n1) * 0.5f;
            float dmr2 = dm.x * dm.x + dm.y * dm.y;
            const float miterLimit = 4.0f;
            if (dmr2 > 1.0e-8f)
            {
                float scale = 1.0f / dmr2;
                if (scale <= miterLimit * miterLimit)
                {
                    dm *= scale;
                    Vector2 tip = vertex + dm * (half * side);
                    addJoinTriangle(primitive, vertex, outer0, tip, uv, color);
                    addJoinTriangle(primitive, vertex, tip, outer1, uv, color);
                    return;
                }
            }
            addJoinTriangle(primitive, vertex, outer0, outer1, uv, color);
        }

        void dashRect(UIPrimitive& primitive, const Rect& area, bool alongX, const UIStrokeStyle& style)
        {
            if (area.size.x <= 0.0f || area.size.y <= 0.0f || style.color.a <= 0.0f)
            {
                return;
            }
            Rect uv = UIFont::solidUV();
            if (style.pattern != UILinePattern::Dashed || style.dashLength <= 0.0f)
            {
                primitive.addQuad(area, uv, style.color);
                return;
            }
            float dash = style.dashLength;
            float gap = style.gapLength > 0.0f ? style.gapLength : dash;
            float length = alongX ? area.size.x : area.size.y;
            float cursor = 0.0f;
            while (cursor < length)
            {
                float on = dash;
                if (cursor + on > length)
                {
                    on = length - cursor;
                }
                Rect piece = area;
                if (alongX)
                {
                    piece.origin.x += cursor;
                    piece.size.x = on;
                }
                else
                {
                    piece.origin.y += cursor;
                    piece.size.y = on;
                }
                primitive.addQuad(piece, uv, style.color);
                cursor += on + gap;
            }
        }
    }

    void UIStroke::segment(UIPrimitive& primitive, const Vector2& a, const Vector2& b, const UIStrokeStyle& style)
    {
        if (style.thickness <= 0.0f || style.color.a <= 0.0f)
        {
            return;
        }
        Vector2 edge(b.x - a.x, b.y - a.y);
        float len = edge.magnitude();
        if (len == 0.0f)
        {
            return;
        }
        float half = style.thickness * 0.5f;
        Vector2 normal((-edge.y / len) * half, (edge.x / len) * half);
        primitive.addQuad(a + normal, b + normal, b - normal, a - normal, UIFont::solidUV(), style.color);
    }

    void UIStroke::path(UIPrimitive& primitive, const std::vector<Vector2>& points, bool closed, const UIStrokeStyle& style)
    {
        if (points.size() < 2 || style.thickness <= 0.0f || style.color.a <= 0.0f)
        {
            return;
        }
        size_t count = points.size();
        size_t edges = closed ? count : count - 1;
        bool dashed = style.pattern == UILinePattern::Dashed && style.dashLength > 0.0f;
        if (!dashed)
        {
            std::vector<Vector2> pts;
            pts.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                if (!pts.empty() && nearPoint(pts.back(), points[i]))
                {
                    continue;
                }
                pts.push_back(points[i]);
            }
            if (closed && pts.size() >= 2 && nearPoint(pts.front(), pts.back()))
            {
                pts.pop_back();
            }
            if (pts.size() < 2)
            {
                return;
            }
            bool loop = closed && pts.size() >= 3;
            size_t n = pts.size();
            size_t edgeCount = loop ? n : n - 1;
            std::vector<Vector2> dirs(edgeCount);
            for (size_t i = 0; i < edgeCount; ++i)
            {
                Vector2 delta = pts[(i + 1) % n] - pts[i];
                float len = delta.magnitude();
                dirs[i] = delta * (1.0f / len);
                segment(primitive, pts[i], pts[(i + 1) % n], style);
            }
            float half = style.thickness * 0.5f;
            if (loop)
            {
                for (size_t i = 0; i < n; ++i)
                {
                    size_t incoming = (i + n - 1) % n;
                    addCorner(primitive, pts[i], dirs[incoming], dirs[i], half, style.color);
                }
            }
            else if (n >= 3)
            {
                for (size_t i = 1; i + 1 < n; ++i)
                {
                    addCorner(primitive, pts[i], dirs[i - 1], dirs[i], half, style.color);
                }
            }
            return;
        }
        UIStrokeStyle solid = solidOf(style);
        float dash = style.dashLength;
        float gap = style.gapLength > 0.0f ? style.gapLength : dash;
        float period = dash + gap;
        float cursor = 0.0f;
        for (size_t edge = 0; edge < edges; ++edge)
        {
            Vector2 a = points[edge];
            Vector2 b = points[(edge + 1) % count];
            Vector2 delta = b - a;
            float len = delta.magnitude();
            if (len == 0.0f)
            {
                continue;
            }
            Vector2 dir = delta * (1.0f / len);
            float walked = 0.0f;
            while (walked < len)
            {
                float into = cursor - floorf(cursor / period) * period;
                float remain = len - walked;
                if (into < dash)
                {
                    float draw = dash - into;
                    if (draw > remain)
                    {
                        draw = remain;
                    }
                    segment(primitive, a + dir * walked, a + dir * (walked + draw), solid);
                    walked += draw;
                    cursor += draw;
                }
                else
                {
                    float skip = period - into;
                    if (skip > remain)
                    {
                        skip = remain;
                    }
                    walked += skip;
                    cursor += skip;
                }
            }
        }
    }

    void UIStroke::border(UIPrimitive& primitive, const Rect& area, const UIStrokeStyle& style)
    {
        float t = style.thickness;
        if (t <= 0.0f || area.size.x <= 0.0f || area.size.y <= 0.0f || style.color.a <= 0.0f)
        {
            return;
        }
        if (t > area.size.y * 0.5f)
        {
            t = area.size.y * 0.5f;
        }
        if (t > area.size.x * 0.5f)
        {
            t = area.size.x * 0.5f;
        }
        float midH = area.size.y - t * 2.0f;
        if (midH < 0.0f)
        {
            midH = 0.0f;
        }
        dashRect(primitive, Rect(area.origin.x, area.origin.y, area.size.x, t), true, style);
        dashRect(primitive, Rect(area.origin.x, area.origin.y + area.size.y - t, area.size.x, t), true, style);
        dashRect(primitive, Rect(area.origin.x, area.origin.y + t, t, midH), false, style);
        dashRect(primitive, Rect(area.origin.x + area.size.x - t, area.origin.y + t, t, midH), false, style);
    }

    void UIStroke::marker(UIPrimitive& primitive, const Vector2& tip, const Vector2& outward, const UIEndpointStyle& style)
    {
        if (style.kind == UIEndpointKind::None || style.size <= 0.0f)
        {
            return;
        }
        float len = outward.magnitude();
        if (len == 0.0f)
        {
            return;
        }
        Vector2 dir = outward * (1.0f / len);
        Vector2 perp(-dir.y, dir.x);
        float size = style.size;
        float wing = size * 0.45f;
        Vector2 back = tip - dir * size;
        Vector2 left = back + perp * wing;
        Vector2 right = back - perp * wing;
        Rect uv = UIFont::solidUV();
        switch (style.kind)
        {
        case UIEndpointKind::TriangleFilled:
        {
            Vector2 vertices[3] = { tip, left, right };
            primitive.addTriangles(vertices, 1, uv, style.fill);
            break;
        }
        case UIEndpointKind::TriangleHollow:
        {
            std::vector<Vector2> loop;
            loop.push_back(tip);
            loop.push_back(left);
            loop.push_back(right);
            path(primitive, loop, true, style.stroke);
            break;
        }
        case UIEndpointKind::ArrowFilled:
        {
            Vector2 notch = tip - dir * (size * 0.62f);
            Vector2 vertices[6] = { tip, left, notch, tip, notch, right };
            primitive.addTriangles(vertices, 2, uv, style.fill);
            break;
        }
        case UIEndpointKind::ArrowHollow:
            segment(primitive, tip, left, style.stroke);
            segment(primitive, tip, right, style.stroke);
            break;
        case UIEndpointKind::CircleFilled:
            addDisc(primitive, tip, size * 0.5f, style.fill);
            break;
        case UIEndpointKind::CircleHollow:
            addRing(primitive, tip, size * 0.5f, style.stroke.thickness, style.stroke.color);
            break;
        case UIEndpointKind::DiamondFilled:
        {
            Vector2 north = tip + dir * (size * 0.5f);
            Vector2 south = tip - dir * (size * 0.5f);
            Vector2 east = tip + perp * (size * 0.5f);
            Vector2 west = tip - perp * (size * 0.5f);
            Vector2 vertices[6] = { north, east, south, north, south, west };
            primitive.addTriangles(vertices, 2, uv, style.fill);
            break;
        }
        case UIEndpointKind::DiamondHollow:
        {
            std::vector<Vector2> loop;
            loop.push_back(tip + dir * (size * 0.5f));
            loop.push_back(tip + perp * (size * 0.5f));
            loop.push_back(tip - dir * (size * 0.5f));
            loop.push_back(tip - perp * (size * 0.5f));
            path(primitive, loop, true, style.stroke);
            break;
        }
        case UIEndpointKind::None:
            break;
        }
    }
}
