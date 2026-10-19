#include "UIChart.h"
#include "UICanvas.h"
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
        pickable = true;
        dragable = true;
        stroke.color = Color(0.95f, 0.97f, 1.0f, 1.0f);
        stroke.thickness = 2.0f;
    }

    Color UIChart::activeFill() const
    {
        if (isPressed())
        {
            return pressedFill;
        }
        if (isHovered())
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
            this->attachChild(mLabel);
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
        t->layout();
        float textW = t->shape.size.x;
        float textH = t->shape.size.y;
        float innerW = shape.size.x - labelPadding * 2.0f;
        float innerH = shape.size.y - labelPadding * 2.0f;
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
        this->placeChild(*t, Vector2(floorf(x + 0.5f), floorf(y + 0.5f)));
    }

    void UIChart::setContour(const std::vector<Vector2>& points)
    {
        mContour.clear();
        if (points.empty())
        {
            Vector2 topLeft = shape.origin - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
            shape.setBox(topLeft, Vector2::ZERO);
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
        shape.setBox(Vector2(minX, minY), Vector2(maxX - minX, maxY - minY));
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
        Vector2 local = point;
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

    UIWidget* UIChart::pick(const Vector2& point)
    {
        if (!visible || shape.scale.x == 0.0f || shape.scale.y == 0.0f)
        {
            return nullptr;
        }
        Vector2 local = shape.pivotToLocal(point);
        if (!this->contains(local))
        {
            return nullptr;
        }
        return UIWidget::pick(point);
    }

    void UIChart::addChart(const std::shared_ptr<UIChart>& chart)
    {
        if (chart)
        {
            this->attachChild(chart);
        }
    }

    void UIChart::removeChart(UIChart* chart)
    {
        this->detachChild(chart);
    }

    void UIChart::scaleAt(const Vector2& focal, float value)
    {
        float previous = shape.scale.x;
        float next = clampScale(value, minScale, maxScale);
        shape.setScaleAround(focal, Vector2(next, next));
        if (next != previous && onZoom)
        {
            onZoom(value - previous, focal.x, focal.y);
        }
    }

    void UIChart::strokeLoop(UIPrimitive& primitive, const Matrix3& localToScreen, const std::vector<Vector2>& localPoints) const
    {
        if (!selected || stroke.thickness <= 0.0f || localPoints.size() < 3)
        {
            return;
        }
        UIStroke::path(primitive, localToScreen, localPoints, true, stroke);
    }

    void UIChart::layout()
    {
        if (!visible)
        {
            return;
        }
        this->placeLabel();
        for (auto& child : children())
        {
            if (!child || child.get() == mLabel.get())
            {
                continue;
            }
            child->layout();
        }
    }

    void UIChart::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Matrix3 localToScreen = matrixLocalToScreen();
        Color fill = this->activeFill();
        std::vector<Vector2> triangles;
        if (triangulate(mContour, triangles))
        {
            std::vector<Vector2> vertices;
            vertices.reserve(triangles.size());
            for (const Vector2& point : triangles)
            {
                vertices.push_back(this->localToScreen(point));
            }
            primitive.addTriangles(vertices.data(), (uint32_t)(vertices.size() / 3), UIFont::solidUV(), fill);
        }
        this->strokeLoop(primitive, localToScreen, mContour);
        UIWidget::render(primitive);
    }

    bool UIChart::handleClick(float screenX, float screenY)
    {
        (void)screenX;
        (void)screenY;
        if (UICanvas* canvas = dynamic_cast<UICanvas*>(parent()))
        {
            canvas->applyCanvasSelection(this);
        }
        return UIWidget::handleClick(screenX, screenY) || true;
    }

    bool UIChart::handleDrop(float screenX, float screenY, UIWidget* hitUnderCursor, const Vector2& delta)
    {
        (void)delta;
        UIWidget* target = nullptr;
        if (hitUnderCursor != nullptr && hitUnderCursor != this)
        {
            UIChart* chartHit = dynamic_cast<UIChart*>(hitUnderCursor);
            if (chartHit != nullptr && chartHit != this)
            {
                target = chartHit;
            }
        }
        if (onChartDrop)
        {
            Vector2 dropPoint = parent() != nullptr ? parent()->screenToPivot(Vector2(screenX, screenY)) : Vector2(screenX, screenY);
            onChartDrop(target, dropPoint.x, dropPoint.y);
            return true;
        }
        return UIWidget::handleDrop(screenX, screenY, hitUnderCursor, delta);
    }

    bool UIChart::handleWheel(float screenX, float screenY, float deltaX, float deltaY)
    {
        (void)deltaX;
        if (shape.scale.x == 0.0f || shape.scale.y == 0.0f)
        {
            return false;
        }
        Vector2 focal = parent() != nullptr ? parent()->screenToPivot(Vector2(screenX, screenY)) : Vector2(screenX, screenY);
        Vector2 local = shape.pivotToLocal(focal);
        if (!this->contains(local))
        {
            return false;
        }
        float current = shape.scale.x == 0.0f ? 1.0f : shape.scale.x;
        float factor = expf(-deltaY * scaleSensitivity);
        this->scaleAt(focal, current * factor);
        return true;
    }
}
