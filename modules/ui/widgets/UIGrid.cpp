#include "UIGrid.h"
#include "../UIFont.h"
#include <cmath>
#include <vector>

namespace eokas
{
    void UIGrid::addChild(const std::shared_ptr<UIWidget>& child)
    {
        children.push_back(child);
    }

    void UIGrid::layout(const Rect& rect)
    {
        {
            Rect _box = rect;
            shape.size = _box.size;
            shape.origin = _box.origin + shape.pivot * shape.size;
        }
        int cols = columns < 1 ? 1 : columns;
        float gapX = spacing;
        float gapY = rowSpacing < 0.0f ? spacing : rowSpacing;
        float pad = padding;

        std::vector<UIWidget*> items;
        float maxW = 0.0f;
        for (auto& child : children)
        {
            if (!child || !child->visible)
            {
                continue;
            }
            items.push_back(child.get());
            if (child->shape.size.x > maxW)
            {
                maxW = child->shape.size.x;
            }
        }

        float innerW = rect.size.x - pad * 2.0f;
        float cellW = cellWidth;
        if (cellW <= 0.0f)
        {
            if (innerW > 0.0f)
            {
                float gaps = gapX * (float)(cols - 1);
                cellW = (innerW - gaps) / (float)cols;
                if (cellW < 0.0f)
                {
                    cellW = 0.0f;
                }
            }
            else
            {
                cellW = maxW;
            }
        }

        int count = (int)items.size();
        int rows = count > 0 ? (count + cols - 1) / cols : 0;
        float y = pad;
        for (int row = 0; row < rows; ++row)
        {
            if (row > 0)
            {
                y += gapY;
            }
            float rowH = cellHeight > 0.0f ? cellHeight : 0.0f;
            for (int col = 0; col < cols; ++col)
            {
                int index = row * cols + col;
                if (index >= count)
                {
                    break;
                }
                UIWidget* item = items[index];
                Vector2 cell(pad + (cellW + gapX) * (float)col, y);
                item->layout(Rect(Vector2(floorf(cell.x + 0.5f), floorf(cell.y + 0.5f)), item->shape.size));
                if (cellHeight <= 0.0f && item->shape.size.y > rowH)
                {
                    rowH = item->shape.size.y;
                }
            }
            y += rowH;
        }

        if (rect.size.x <= 0.0f)
        {
            float gaps = cols > 1 ? gapX * (float)(cols - 1) : 0.0f;
            shape.origin.x += shape.pivot.x * ((pad * 2.0f + cellW * (float)cols + gaps) - shape.size.x);
            shape.size.x = pad * 2.0f + cellW * (float)cols + gaps;
        }
        if (rect.size.y <= 0.0f)
        {
            shape.origin.y += shape.pivot.y * ((y + pad) - shape.size.y);
            shape.size.y = y + pad;
        }
    }

    void UIGrid::refit()
    {
        int cols = columns < 1 ? 1 : columns;
        float gapX = spacing;
        float gapY = rowSpacing < 0.0f ? spacing : rowSpacing;
        float pad = padding;

        std::vector<UIWidget*> items;
        float maxW = 0.0f;
        for (auto& child : children)
        {
            if (!child || !child->visible)
            {
                continue;
            }
            items.push_back(child.get());
            if (child->shape.size.x > maxW)
            {
                maxW = child->shape.size.x;
            }
        }

        float cellW = cellWidth;
        bool writeWidth = cellW > 0.0f || shape.size.x <= 0.0f;
        if (cellW <= 0.0f)
        {
            float innerW = shape.size.x - pad * 2.0f;
            if (innerW > 0.0f)
            {
                float gaps = gapX * (float)(cols - 1);
                cellW = (innerW - gaps) / (float)cols;
                if (cellW < 0.0f)
                {
                    cellW = 0.0f;
                }
            }
            else
            {
                cellW = maxW;
            }
        }

        int count = (int)items.size();
        int rows = count > 0 ? (count + cols - 1) / cols : 0;
        float contentH = pad;
        for (int row = 0; row < rows; ++row)
        {
            if (row > 0)
            {
                contentH += gapY;
            }
            float rowH = cellHeight > 0.0f ? cellHeight : 0.0f;
            if (cellHeight <= 0.0f)
            {
                for (int col = 0; col < cols; ++col)
                {
                    int index = row * cols + col;
                    if (index >= count)
                    {
                        break;
                    }
                    if (items[index]->shape.size.y > rowH)
                    {
                        rowH = items[index]->shape.size.y;
                    }
                }
            }
            contentH += rowH;
        }
        contentH += pad;
        shape.origin.y += shape.pivot.y * ((contentH) - shape.size.y);
        shape.size.y = contentH;

        if (writeWidth)
        {
            float gaps = cols > 1 ? gapX * (float)(cols - 1) : 0.0f;
            shape.origin.x += shape.pivot.x * ((pad * 2.0f + cellW * (float)cols + gaps) - shape.size.x);
            shape.size.x = pad * 2.0f + cellW * (float)cols + gaps;
        }
    }

    void UIGrid::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(shape.origin, shape.scale);
        if (color.a > 0.0f)
        {
            primitive.addQuad(Rect(shape.left(), shape.top(), shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
