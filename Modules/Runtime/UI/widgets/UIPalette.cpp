#include "UIPalette.h"
#include "../UIFont.h"
#include "../UIStroke.h"
#include <cmath>

namespace eokas
{
    namespace
    {
        struct Metrics
        {
            float pad = 0.0f;
            float gap = 0.0f;
            float cell = 0.0f;
        };

        float snapf(float v)
        {
            return floorf(v + 0.5f);
        }

        void hsvToRgb(float h, float s, float v, float& r, float& g, float& b)
        {
            h = fmodf(h, 360.0f);
            if (h < 0.0f) h += 360.0f;
            float c = v * s;
            float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
            float m = v - c;
            float rp = 0.0f;
            float gp = 0.0f;
            float bp = 0.0f;
            int sector = (int)floorf(h / 60.0f);
            if (sector < 0) sector = 0;
            if (sector > 5) sector = 5;
            switch (sector)
            {
            case 0: rp = c; gp = x; bp = 0.0f; break;
            case 1: rp = x; gp = c; bp = 0.0f; break;
            case 2: rp = 0.0f; gp = c; bp = x; break;
            case 3: rp = 0.0f; gp = x; bp = c; break;
            case 4: rp = x; gp = 0.0f; bp = c; break;
            default: rp = c; gp = 0.0f; bp = x; break;
            }
            r = rp + m;
            g = gp + m;
            b = bp + m;
        }

        Metrics metricsOf(float scale, float padding, float spacing, float cellSize)
        {
            if (!(scale > 0.0f)) scale = 1.0f;
            Metrics m;
            m.pad = snapf(padding * scale);
            m.gap = snapf(spacing * scale);
            m.cell = snapf(cellSize * scale);
            if (m.cell < 1.0f) m.cell = 1.0f;
            return m;
        }

        Metrics fittedOf(float scale, float padding, float spacing, float cellSize, float width)
        {
            Metrics m = metricsOf(scale, padding, spacing, cellSize);
            if (!(width > 0.0f)) return m;
            float inner = width - m.pad * 2.0f;
            if (!(inner > 0.0f)) return m;
            float cell = floorf((inner - m.gap * (float)(UIPalette::kColumns - 1)) / (float)UIPalette::kColumns);
            if (cell < 1.0f) cell = 1.0f;
            m.cell = cell;
            return m;
        }

        Vector2 sizeOf(const Metrics& m)
        {
            return Vector2(
                m.pad * 2.0f + m.cell * (float)UIPalette::kColumns + m.gap * (float)(UIPalette::kColumns - 1),
                m.pad * 2.0f + m.cell * (float)UIPalette::kRows + m.gap * (float)(UIPalette::kRows - 1));
        }

        void paintChecker(UIPrimitive& primitive, const Matrix3& localToScreen, const Rect& area, float scale)
        {
            if (area.size.x <= 0.0f || area.size.y <= 0.0f) return;
            float cell = 6.0f * scale;
            if (cell < 4.0f) cell = 4.0f;
            int cols = (int)ceilf(area.size.x / cell);
            int rows = (int)ceilf(area.size.y / cell);
            if (cols < 1) cols = 1;
            if (rows < 1) rows = 1;
            if (cols > 16) cols = 16;
            if (rows > 16) rows = 16;
            float cw = area.size.x / (float)cols;
            float ch = area.size.y / (float)rows;
            Color light(0.86f, 0.86f, 0.86f, 1.0f);
            Color dark(0.58f, 0.58f, 0.58f, 1.0f);
            Rect uv = UIFont::solidUV();
            for (int row = 0; row < rows; ++row)
            {
                for (int col = 0; col < cols; ++col)
                {
                    Color fill = ((col + row) & 1) ? dark : light;
                    primitive.addQuad(
                        localToScreen,
                        Rect(area.origin.x + cw * (float)col, area.origin.y + ch * (float)row, cw, ch),
                        uv,
                        fill);
                }
            }
        }
    }

    UIPalette::UIPalette()
    {
        pickable = true;
        UIWidget::color.a = 0.0f;
        mGrid = std::make_shared<UIGrid>();
        mGrid->pickable = false;
        mGrid->columns = kColumns;
        mGrid->color.a = 0.0f;
        this->attachChild(mGrid);
        for (int i = 0; i < kCount; ++i)
        {
            auto cell = std::make_shared<UIWidget>();
            cell->color = paletteColor(i);
            cell->onClick = [this, i]() { this->setIndex(i); };
            mGrid->attachChild(cell);
        }
        mIndex = 0;
        mColor = paletteColor(0);
    }

    Vector2 UIPalette::preferredSize(float scale, float padding, float spacing, float cellSize)
    {
        return sizeOf(metricsOf(scale, padding, spacing, cellSize));
    }

    Color UIPalette::paletteColor(int index)
    {
        if (index < 0) index = 0;
        if (index >= kCount) index = kCount - 1;
        int row = index / kColumns;
        int col = index % kColumns;
        static const float kHues[kColumns] = { 0.0f, 0.0f, 30.0f, 60.0f, 120.0f, 180.0f, 240.0f, 270.0f };
        static const float kValues[kRows] = { 1.00f, 0.80f, 0.60f, 0.40f, 0.20f };
        static const float kGrays[kRows - 1] = { 1.00f, 0.67f, 0.33f, 0.00f };
        if (col == 0)
        {
            if (row == 0) return Color(0.0f, 0.0f, 0.0f, 0.0f);
            float g = kGrays[row - 1];
            return Color(g, g, g, 1.0f);
        }
        float v = kValues[row];
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        hsvToRgb(kHues[col], 1.0f, v, r, g, b);
        return Color(r, g, b, 1.0f);
    }

    void UIPalette::setColor(const Color& value)
    {
        int best = 0;
        float bestDist = 0.0f;
        for (int i = 0; i < kCount; ++i)
        {
            Color swatch = paletteColor(i);
            float dr = swatch.r - value.r;
            float dg = swatch.g - value.g;
            float db = swatch.b - value.b;
            float da = swatch.a - value.a;
            float dist = dr * dr + dg * dg + db * db + da * da;
            if (i == 0 || dist < bestDist)
            {
                best = i;
                bestDist = dist;
            }
        }
        this->setIndex(best);
    }

    void UIPalette::setIndex(int index)
    {
        if (index < 0) index = 0;
        if (index >= kCount) index = kCount - 1;
        if (index == mIndex) return;
        mIndex = index;
        mColor = paletteColor(index);
        this->notifyChanged();
    }

    void UIPalette::layout()
    {
        if (!visible) return;
        Metrics m = fittedOf(this->scaleOf(), padding, spacing, cellSize, shape.size.x);
        Vector2 need = sizeOf(m);
        if (shape.size.x <= 0.0f || shape.size.y <= 0.0f)
        {
            this->resize(Vector2(
                shape.size.x > 0.0f ? shape.size.x : need.x,
                shape.size.y > 0.0f ? shape.size.y : need.y));
        }
        if (!mGrid) return;
        mGrid->columns = kColumns;
        mGrid->padding = m.pad;
        mGrid->spacing = m.gap;
        mGrid->cellWidth = m.cell;
        mGrid->cellHeight = m.cell;
        for (const auto& child : mGrid->children())
        {
            if (!child) continue;
            child->shape.size = Vector2(m.cell, m.cell);
        }
        this->placeChild(*mGrid, Vector2(0.0f, 0.0f), shape.size);
        UIWidget::layout();
    }

    void UIPalette::render(UIPrimitive& primitive)
    {
        if (!visible) return;
        Matrix3 localToScreen = this->matrixLocalToScreen();
        primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), background);
        UIStrokeStyle border = line;
        border.thickness = this->scaleOf();
        if (border.thickness < 1.0f) border.thickness = 1.0f;
        UIStroke::border(primitive, localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), border);
        paintChecker(primitive, localToScreen, this->cellRect(0), this->scaleOf());
        UIWidget::render(primitive);

        Rect selected = this->cellRect(mIndex);
        if (selected.size.x > 0.0f && selected.size.y > 0.0f)
        {
            UIStrokeStyle mark = accent;
            mark.thickness = border.thickness;
            UIStroke::border(primitive, localToScreen, selected, mark);
        }
    }

    bool UIPalette::handleClick(float screenX, float screenY)
    {
        Vector2 local = this->screenToLocal(Vector2(screenX, screenY));
        int index = this->hitIndex(local.x, local.y);
        if (index >= 0)
        {
            this->setIndex(index);
            return true;
        }
        return UIWidget::handleClick(screenX, screenY);
    }

    float UIPalette::scaleOf() const
    {
        return uiScale > 0.0f ? uiScale : 1.0f;
    }

    int UIPalette::hitIndex(float localX, float localY) const
    {
        Metrics m = fittedOf(this->scaleOf(), padding, spacing, cellSize, shape.size.x);
        float x = localX - m.pad;
        float y = localY - m.pad;
        if (x < 0.0f || y < 0.0f) return -1;
        float stride = m.cell + m.gap;
        if (stride <= 0.0f) return -1;
        int col = (int)floorf(x / stride);
        int row = (int)floorf(y / stride);
        if (col < 0 || col >= kColumns || row < 0 || row >= kRows) return -1;
        float lx = x - (float)col * stride;
        float ly = y - (float)row * stride;
        if (lx > m.cell || ly > m.cell) return -1;
        return row * kColumns + col;
    }

    Rect UIPalette::cellRect(int index) const
    {
        if (index < 0 || index >= kCount) return Rect();
        Metrics m = fittedOf(this->scaleOf(), padding, spacing, cellSize, shape.size.x);
        int row = index / kColumns;
        int col = index % kColumns;
        float stride = m.cell + m.gap;
        return Rect(m.pad + stride * (float)col, m.pad + stride * (float)row, m.cell, m.cell);
    }

    void UIPalette::notifyChanged()
    {
        if (mSilence || !onColorChanged) return;
        mSilence = true;
        Color copy = mColor;
        onColorChanged(copy);
        mSilence = false;
    }
}
