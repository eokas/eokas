#include "UILink.h"
#include "../UIStroke.h"

#include <cmath>
#include <vector>

namespace eokas
{
    namespace
    {
        float lesser(float a, float b)
        {
            return a < b ? a : b;
        }

        float greater(float a, float b)
        {
            return a > b ? a : b;
        }

        Vector2 chartCenter(const UIChart* chart)
        {
            return Vector2(chart->shape.left(), chart->shape.top()) + chart->shape.size * 0.5f;
        }

        UIAnchor resolvedAnchor(const UIChart* chart, UIAnchor anchor, const Vector2& other)
        {
            if (anchor != UIAnchor::Auto)
            {
                return anchor;
            }
            Vector2 dir = other - chartCenter(chart);
            float ax = dir.x < 0.0f ? -dir.x : dir.x;
            float ay = dir.y < 0.0f ? -dir.y : dir.y;
            if (ax >= ay)
            {
                return dir.x >= 0.0f ? UIAnchor::Right : UIAnchor::Left;
            }
            return dir.y >= 0.0f ? UIAnchor::Bottom : UIAnchor::Top;
        }

        Vector2 anchorOutward(UIAnchor side)
        {
            if (side == UIAnchor::Top)
            {
                return Vector2(0.0f, -1.0f);
            }
            if (side == UIAnchor::Bottom)
            {
                return Vector2(0.0f, 1.0f);
            }
            if (side == UIAnchor::Left)
            {
                return Vector2(-1.0f, 0.0f);
            }
            if (side == UIAnchor::Right)
            {
                return Vector2(1.0f, 0.0f);
            }
            return Vector2::ZERO;
        }

        Vector2 anchorPoint(const UIChart* chart, UIAnchor anchor, const Vector2& other)
        {
            Rect area = Rect(chart->shape.left(), chart->shape.top(), chart->shape.size.x, chart->shape.size.y);
            Vector2 center = chartCenter(chart);
            UIAnchor side = resolvedAnchor(chart, anchor, other);
            if (side == UIAnchor::Top)
            {
                return Vector2(center.x, area.origin.y);
            }
            if (side == UIAnchor::Bottom)
            {
                return Vector2(center.x, area.origin.y + area.size.y);
            }
            if (side == UIAnchor::Left)
            {
                return Vector2(area.origin.x, center.y);
            }
            if (side == UIAnchor::Right)
            {
                return Vector2(area.origin.x + area.size.x, center.y);
            }
            return center;
        }

        void placeHandle(Vector2& handle, const Vector2& endpoint, const Vector2& outward)
        {
            float along = (handle.x - endpoint.x) * outward.x + (handle.y - endpoint.y) * outward.y;
            if (along < 0.0f)
            {
                along = -along;
            }
            if (along < 8.0f)
            {
                along = 8.0f;
            }
            handle = endpoint + outward * along;
        }

        Vector2 cubicPoint(const Vector2& p0, const Vector2& c1, const Vector2& c2, const Vector2& p1, float t)
        {
            float u = 1.0f - t;
            return p0 * (u * u * u) + c1 * (3.0f * u * u * t) + c2 * (3.0f * u * t * t) + p1 * (t * t * t);
        }

        void flattenCubic(const std::vector<Vector2>& controls, int steps, std::vector<Vector2>& out)
        {
            out.clear();
            if (controls.size() < 4)
            {
                out = controls;
                return;
            }
            int count = steps < 1 ? 1 : steps;
            size_t index = 0;
            while (index + 3 < controls.size())
            {
                const Vector2& p0 = controls[index];
                const Vector2& c1 = controls[index + 1];
                const Vector2& c2 = controls[index + 2];
                const Vector2& p1 = controls[index + 3];
                for (int step = 0; step <= count; ++step)
                {
                    if (step == 0 && !out.empty())
                    {
                        continue;
                    }
                    out.push_back(cubicPoint(p0, c1, c2, p1, (float)step / (float)count));
                }
                index += 3;
            }
        }

        float markerInset(const UIEndpointStyle& cap)
        {
            if (cap.kind == UIEndpointKind::None || cap.size <= 0.0f)
            {
                return 0.0f;
            }
            if (cap.kind == UIEndpointKind::CircleFilled || cap.kind == UIEndpointKind::CircleHollow
                || cap.kind == UIEndpointKind::DiamondFilled || cap.kind == UIEndpointKind::DiamondHollow)
            {
                return cap.size * 0.5f;
            }
            return cap.size;
        }

        void trimEnd(std::vector<Vector2>& points, float amount, bool fromStart)
        {
            if (amount <= 0.0f || points.size() < 2)
            {
                return;
            }
            if (fromStart)
            {
                std::vector<Vector2> reversed;
                reversed.reserve(points.size());
                for (size_t i = points.size(); i > 0; --i)
                {
                    reversed.push_back(points[i - 1]);
                }
                points.swap(reversed);
            }
            float remain = amount;
            while (points.size() >= 2 && remain > 0.0f)
            {
                Vector2 delta = points.back() - points[points.size() - 2];
                float len = delta.magnitude();
                if (len == 0.0f)
                {
                    points.pop_back();
                    continue;
                }
                if (len <= remain)
                {
                    points.pop_back();
                    remain -= len;
                    continue;
                }
                points.back() = points.back() - delta * (remain / len);
                remain = 0.0f;
            }
            if (fromStart)
            {
                std::vector<Vector2> reversed;
                reversed.reserve(points.size());
                for (size_t i = points.size(); i > 0; --i)
                {
                    reversed.push_back(points[i - 1]);
                }
                points.swap(reversed);
            }
        }

        Vector2 outwardAt(const std::vector<Vector2>& points, bool atStart)
        {
            if (points.size() < 2)
            {
                return Vector2(1.0f, 0.0f);
            }
            Vector2 delta = atStart ? points[0] - points[1] : points.back() - points[points.size() - 2];
            float len = delta.magnitude();
            if (len == 0.0f)
            {
                return Vector2(1.0f, 0.0f);
            }
            return delta * (1.0f / len);
        }

        float distanceToSegment(const Vector2& point, const Vector2& a, const Vector2& b)
        {
            Vector2 ab = b - a;
            float len2 = ab.x * ab.x + ab.y * ab.y;
            if (len2 == 0.0f)
            {
                return (point - a).magnitude();
            }
            float t = ((point.x - a.x) * ab.x + (point.y - a.y) * ab.y) / len2;
            if (t < 0.0f)
            {
                t = 0.0f;
            }
            if (t > 1.0f)
            {
                t = 1.0f;
            }
            Vector2 closest = a + ab * t;
            return (point - closest).magnitude();
        }
    }

    UILink::UILink()
    {
        line.color = Color(0.95f, 0.97f, 1.0f, 1.0f);
        line.thickness = 2.0f;
    }

    void UILink::setPoints(const std::vector<Vector2>& parentPoints)
    {
        mLocal.clear();
        if (parentPoints.empty())
        {
            shape.origin += shape.pivot * ((Vector2::ZERO) - shape.size);
            shape.size = Vector2::ZERO;
            return;
        }
        float minX = parentPoints[0].x;
        float minY = parentPoints[0].y;
        float maxX = minX;
        float maxY = minY;
        for (const Vector2& point : parentPoints)
        {
            minX = lesser(minX, point.x);
            minY = lesser(minY, point.y);
            maxX = greater(maxX, point.x);
            maxY = greater(maxY, point.y);
        }
        shape.origin = (Vector2(minX, minY)) + shape.pivot * shape.size;
        shape.origin += shape.pivot * ((Vector2(maxX - minX, maxY - minY)) - shape.size);
        shape.size = Vector2(maxX - minX, maxY - minY);
        mLocal.reserve(parentPoints.size());
        for (const Vector2& point : parentPoints)
        {
            mLocal.push_back(Vector2(point.x - minX, point.y - minY));
        }
    }

    void UILink::resolve(std::vector<Vector2>& parent) const
    {
        parent.clear();
        std::vector<Vector2> raw;
        raw.reserve(mLocal.size());
        for (const Vector2& point : mLocal)
        {
            raw.push_back(Vector2(shape.left(), shape.top()) + point);
        }
        if (kind == UIPathKind::Straight)
        {
            if (!raw.empty())
            {
                parent.push_back(raw.front());
            }
            if (raw.size() >= 2)
            {
                parent.push_back(raw.back());
            }
        }
        else
        {
            parent = raw;
        }

        Vector2 startOther = parent.empty() ? Vector2(shape.left(), shape.top()) : parent.back();
        if (end.target != nullptr)
        {
            startOther = chartCenter(end.target);
        }
        if (start.target != nullptr)
        {
            Vector2 point = anchorPoint(start.target, start.anchor, startOther);
            if (parent.empty())
            {
                parent.push_back(point);
            }
            else
            {
                parent.front() = point;
            }
        }
        Vector2 endOther = parent.empty() ? Vector2(shape.left(), shape.top()) : parent.front();
        if (end.target != nullptr)
        {
            Vector2 point = anchorPoint(end.target, end.anchor, endOther);
            if (parent.empty())
            {
                parent.push_back(point);
            }
            else if (parent.size() == 1)
            {
                parent.push_back(point);
            }
            else
            {
                parent.back() = point;
            }
        }

        if (kind == UIPathKind::CubicBezier && parent.size() >= 4)
        {
            if (start.target != nullptr)
            {
                Vector2 outward = anchorOutward(resolvedAnchor(start.target, start.anchor, startOther));
                if (outward.x != 0.0f || outward.y != 0.0f)
                {
                    placeHandle(parent[1], parent.front(), outward);
                }
            }
            if (end.target != nullptr)
            {
                Vector2 outward = anchorOutward(resolvedAnchor(end.target, end.anchor, endOther));
                if (outward.x != 0.0f || outward.y != 0.0f)
                {
                    placeHandle(parent[parent.size() - 2], parent.back(), outward);
                }
            }
            std::vector<Vector2> flat;
            flattenCubic(parent, bezierSteps, flat);
            parent.swap(flat);
        }
    }

    void UILink::syncBounds(const std::vector<Vector2>& parent)
    {
        if (parent.empty())
        {
            return;
        }
        float minX = parent[0].x;
        float minY = parent[0].y;
        float maxX = minX;
        float maxY = minY;
        for (const Vector2& point : parent)
        {
            minX = lesser(minX, point.x);
            minY = lesser(minY, point.y);
            maxX = greater(maxX, point.x);
            maxY = greater(maxY, point.y);
        }
        Vector2 newOrigin(minX, minY);
        Vector2 delta = Vector2(shape.left(), shape.top()) - newOrigin;
        for (Vector2& point : mLocal)
        {
            point += delta;
        }
        shape.origin = (newOrigin) + shape.pivot * shape.size;
        shape.origin += shape.pivot * ((Vector2(maxX - minX, maxY - minY)) - shape.size);
        shape.size = Vector2(maxX - minX, maxY - minY);
    }

    bool UILink::contains(const Vector2& point) const
    {
        std::vector<Vector2> parent;
        this->resolve(parent);
        if (parent.size() < 2)
        {
            return false;
        }
        const UIStrokeStyle& drawn = (selected && stroke.thickness > 0.0f) ? stroke : line;
        float limit = hitSlop > drawn.thickness ? hitSlop : drawn.thickness;
        for (size_t i = 1; i < parent.size(); ++i)
        {
            if (distanceToSegment(point, parent[i - 1], parent[i]) <= limit)
            {
                return true;
            }
        }
        return false;
    }

    void UILink::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        std::vector<Vector2> parent;
        this->resolve(parent);
        this->syncBounds(parent);
        primitive.pushScaleAround(shape.origin, shape.scale);
        if (parent.size() >= 2)
        {
            std::vector<Vector2> stroked = parent;
            trimEnd(stroked, markerInset(start.cap), true);
            trimEnd(stroked, markerInset(end.cap), false);
            const UIStrokeStyle& drawn = (selected && stroke.thickness > 0.0f) ? stroke : line;
            UIStroke::path(primitive, stroked, false, drawn);
            UIStroke::marker(primitive, parent.front(), outwardAt(parent, true), start.cap);
            UIStroke::marker(primitive, parent.back(), outwardAt(parent, false), end.cap);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
