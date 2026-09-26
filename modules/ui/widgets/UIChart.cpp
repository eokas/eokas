#include "UIChart.h"
#include "../UIFont.h"
#include "../UIStroke.h"

#include <cmath>
#include <cstddef>
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

        float clampScale(float value, float minScale, float maxScale)
        {
            float lo = minScale;
            float hi = maxScale;
            if (lo < 0.01f)
            {
                lo = 0.01f;
            }
            if (hi < lo)
            {
                hi = lo;
            }
            if (value < lo)
            {
                return lo;
            }
            if (value > hi)
            {
                return hi;
            }
            return value;
        }

        float axisScale(float value)
        {
            return value == 0.0f ? 1.0f : value;
        }

        float cross(const Vector2& a, const Vector2& b, const Vector2& c)
        {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        }

        bool pointInTriangle(const Vector2& p, const Vector2& a, const Vector2& b, const Vector2& c)
        {
            float c0 = cross(a, b, p);
            float c1 = cross(b, c, p);
            float c2 = cross(c, a, p);
            bool hasNeg = c0 < 0.0f || c1 < 0.0f || c2 < 0.0f;
            bool hasPos = c0 > 0.0f || c1 > 0.0f || c2 > 0.0f;
            return !(hasNeg && hasPos);
        }

        bool triangulate(const std::vector<Vector2>& source, std::vector<Vector2>& out)
        {
            out.clear();
            if (source.size() < 3)
            {
                return false;
            }
            std::vector<Vector2> poly = source;
            if (poly.size() > 3 && poly.front().x == poly.back().x && poly.front().y == poly.back().y)
            {
                poly.pop_back();
            }
            if (poly.size() < 3)
            {
                return false;
            }
            float area = 0.0f;
            for (size_t i = 0; i < poly.size(); ++i)
            {
                const Vector2& a = poly[i];
                const Vector2& b = poly[(i + 1) % poly.size()];
                area += a.x * b.y - b.x * a.y;
            }
            if (area < 0.0f)
            {
                std::vector<Vector2> reversed;
                reversed.reserve(poly.size());
                for (size_t i = poly.size(); i > 0; --i)
                {
                    reversed.push_back(poly[i - 1]);
                }
                poly.swap(reversed);
            }
            int guard = (int)poly.size() * (int)poly.size();
            while (poly.size() > 3 && guard-- > 0)
            {
                bool clipped = false;
                for (size_t i = 0; i < poly.size(); ++i)
                {
                    size_t i0 = (i + poly.size() - 1) % poly.size();
                    size_t i2 = (i + 1) % poly.size();
                    const Vector2& a = poly[i0];
                    const Vector2& b = poly[i];
                    const Vector2& c = poly[i2];
                    if (cross(a, b, c) <= 0.0f)
                    {
                        continue;
                    }
                    bool blocked = false;
                    for (size_t j = 0; j < poly.size(); ++j)
                    {
                        if (j == i0 || j == i || j == i2)
                        {
                            continue;
                        }
                        if (pointInTriangle(poly[j], a, b, c))
                        {
                            blocked = true;
                            break;
                        }
                    }
                    if (blocked)
                    {
                        continue;
                    }
                    out.push_back(a);
                    out.push_back(b);
                    out.push_back(c);
                    poly.erase(poly.begin() + (std::ptrdiff_t)i);
                    clipped = true;
                    break;
                }
                if (!clipped)
                {
                    out.clear();
                    return false;
                }
            }
            if (poly.size() == 3)
            {
                out.push_back(poly[0]);
                out.push_back(poly[1]);
                out.push_back(poly[2]);
                return true;
            }
            out.clear();
            return false;
        }
    }

    UIChart::UIChart()
    {
        interactive = true;
        dragable = true;
        stroke.color = Color(0.95f, 0.97f, 1.0f, 1.0f);
        stroke.thickness = 2.0f;
    }

    Color UIChart::activeFill() const
    {
        if (pressed)
        {
            return pressedFill;
        }
        if (hovered)
        {
            return hoverFill;
        }
        return background;
    }

    void UIChart::setText(const String& value)
    {
        if (!mLabel)
        {
            mLabel = std::make_shared<UIText>();
            children.push_back(mLabel);
        }
        mLabel->text = value;
    }

    void UIChart::placeLabel()
    {
        UIText* t = mLabel.get();
        if (t == nullptr)
        {
            return;
        }
        float textW = t->rect.size.x;
        float textH = t->rect.size.y;
        UIFont* font = UIFont::find(t->style.fontPath);
        if (font != nullptr && font->isOpen())
        {
            float scale = 1.0f;
            float ascender = 0.0f;
            float descender = 0.0f;
            font->drawMetrics(t->style.fontSize, scale, ascender, descender);
            textW = 0.0f;
            size_t index = 0;
            while (index < t->text.length())
            {
                uint32_t codepoint = 0;
                if (!UIFont::nextUtf8(t->text.cstr(), t->text.length(), index, codepoint))
                {
                    continue;
                }
                textW += font->glyphSized(codepoint, t->style.fontSize).advance * scale;
            }
            textW = floorf(textW + 0.5f);
            textH = floorf((ascender - descender) + 0.5f);
        }
        float innerW = rect.size.x - labelPadding * 2.0f;
        float innerH = rect.size.y - labelPadding * 2.0f;
        if (innerW < 0.0f)
        {
            innerW = 0.0f;
        }
        if (innerH < 0.0f)
        {
            innerH = 0.0f;
        }
        float x = labelPadding + (innerW - textW) * 0.5f;
        float y = labelPadding + (innerH - textH) * 0.5f;
        t->rect = Rect(Vector2(floorf(x + 0.5f), floorf(y + 0.5f)), Vector2(textW, textH));
    }

    void UIChart::setContour(const std::vector<Vector2>& points)
    {
        mContour.clear();
        if (points.empty())
        {
            rect.size = Vector2::ZERO;
            return;
        }
        float minX = points[0].x;
        float minY = points[0].y;
        float maxX = minX;
        float maxY = minY;
        for (const Vector2& point : points)
        {
            minX = lesser(minX, point.x);
            minY = lesser(minY, point.y);
            maxX = greater(maxX, point.x);
            maxY = greater(maxY, point.y);
        }
        rect.origin = Vector2(minX, minY);
        rect.size = Vector2(maxX - minX, maxY - minY);
        mContour.reserve(points.size());
        for (const Vector2& point : points)
        {
            mContour.push_back(Vector2(point.x - minX, point.y - minY));
        }
    }

    bool UIChart::contains(const Vector2& point) const
    {
        if (mContour.size() < 3)
        {
            return false;
        }
        Vector2 local(point.x - rect.origin.x, point.y - rect.origin.y);
        bool inside = false;
        for (size_t i = 0, j = mContour.size() - 1; i < mContour.size(); j = i++)
        {
            const Vector2& a = mContour[j];
            const Vector2& b = mContour[i];
            float abx = b.x - a.x;
            float aby = b.y - a.y;
            float apx = local.x - a.x;
            float apy = local.y - a.y;
            float side = apx * aby - apy * abx;
            if (side == 0.0f)
            {
                float dot = apx * abx + apy * aby;
                float len2 = abx * abx + aby * aby;
                if (dot >= 0.0f && dot <= len2)
                {
                    return true;
                }
            }
            bool crosses = (a.y > local.y) != (b.y > local.y);
            if (crosses)
            {
                float x = (b.x - a.x) * (local.y - a.y) / (b.y - a.y) + a.x;
                if (local.x < x)
                {
                    inside = !inside;
                }
            }
        }
        return inside;
    }

    void UIChart::addChart(const std::shared_ptr<UIChart>& chart)
    {
        if (chart)
        {
            children.push_back(chart);
        }
    }

    void UIChart::removeChart(UIChart* chart)
    {
        for (auto it = children.begin(); it != children.end(); ++it)
        {
            if (it->get() == chart)
            {
                children.erase(it);
                return;
            }
        }
    }

    void UIChart::scaleAt(const Vector2& pivot, float value)
    {
        float previous = localScale.x;
        float next = clampScale(value, minScale, maxScale);
        float kx = next / axisScale(localScale.x);
        float ky = next / axisScale(localScale.y);
        rect.origin.x = pivot.x - (pivot.x - rect.origin.x) * kx;
        rect.origin.y = pivot.y - (pivot.y - rect.origin.y) * ky;
        localScale = Vector2(next, next);
        if (next != previous && onZoom)
        {
            onZoom(value - previous, pivot.x, pivot.y);
        }
    }

    void UIChart::strokeLoop(UIPrimitive& primitive, const std::vector<Vector2>& localPoints) const
    {
        if (!selected || stroke.thickness <= 0.0f || localPoints.size() < 3)
        {
            return;
        }
        std::vector<Vector2> parent;
        parent.reserve(localPoints.size());
        for (const Vector2& point : localPoints)
        {
            parent.push_back(rect.origin + point);
        }
        UIStroke::path(primitive, parent, true, stroke);
    }

    void UIChart::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        Color fill = this->activeFill();
        std::vector<Vector2> triangles;
        if (triangulate(mContour, triangles))
        {
            std::vector<Vector2> vertices;
            vertices.reserve(triangles.size());
            for (const Vector2& point : triangles)
            {
                vertices.push_back(rect.origin + point);
            }
            primitive.addTriangles(vertices.data(), (uint32_t)(vertices.size() / 3), UIFont::solidUV(), fill);
        }
        this->strokeLoop(primitive, mContour);
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
