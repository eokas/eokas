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

    void UIGrid::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
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

        float innerW = shape.size.x - pad * 2.0f;
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
                this->placeChild(*item, Vector2(floorf(cell.x + 0.5f), floorf(cell.y + 0.5f)));
                if (cellHeight <= 0.0f && item->shape.size.y > rowH)
                {
                    rowH = item->shape.size.y;
                }
            }
            y += rowH;
        }

        if (shape.size.x <= 0.0f)
        {
            float gaps = cols > 1 ? gapX * (float)(cols - 1) : 0.0f;
            this->resize(Vector2(pad * 2.0f + cellW * (float)cols + gaps, shape.size.y));
        }
        if (shape.size.y <= 0.0f)
        {
            this->resize(Vector2(shape.size.x, y + pad));
        }
        if (color.a > 0.0f)
        {
            primitive.addQuad(shape.worldTrans(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }
        UIWidget::render(primitive);
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
        Vector2 fitted(shape.size.x, contentH);
        if (writeWidth)
        {
            float gaps = cols > 1 ? gapX * (float)(cols - 1) : 0.0f;
            fitted.x = pad * 2.0f + cellW * (float)cols + gaps;
        }
        this->resize(fitted);
    }
}
