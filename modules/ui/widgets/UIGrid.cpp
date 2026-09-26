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
        this->rect = rect;
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
            if (child->rect.size.x > maxW)
            {
                maxW = child->rect.size.x;
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
                item->layout(Rect(Vector2(floorf(cell.x + 0.5f), floorf(cell.y + 0.5f)), item->rect.size));
                if (cellHeight <= 0.0f && item->rect.size.y > rowH)
                {
                    rowH = item->rect.size.y;
                }
            }
            y += rowH;
        }

        if (rect.size.x <= 0.0f)
        {
            float gaps = cols > 1 ? gapX * (float)(cols - 1) : 0.0f;
            this->rect.size.x = pad * 2.0f + cellW * (float)cols + gaps;
        }
        if (rect.size.y <= 0.0f)
        {
            this->rect.size.y = y + pad;
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
            if (child->rect.size.x > maxW)
            {
                maxW = child->rect.size.x;
            }
        }

        float cellW = cellWidth;
        bool writeWidth = cellW > 0.0f || rect.size.x <= 0.0f;
        if (cellW <= 0.0f)
        {
            float innerW = rect.size.x - pad * 2.0f;
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
                    if (items[index]->rect.size.y > rowH)
                    {
                        rowH = items[index]->rect.size.y;
                    }
                }
            }
            contentH += rowH;
        }
        contentH += pad;
        rect.size.y = contentH;

        if (writeWidth)
        {
            float gaps = cols > 1 ? gapX * (float)(cols - 1) : 0.0f;
            rect.size.x = pad * 2.0f + cellW * (float)cols + gaps;
        }
    }

    void UIGrid::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        if (color.a > 0.0f)
        {
            primitive.addQuad(rect, UIFont::solidUV(), color);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
