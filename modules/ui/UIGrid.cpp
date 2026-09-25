#include "UIGrid.h"
#include "UIFont.h"
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
            if (child->rect.width > maxW)
            {
                maxW = child->rect.width;
            }
        }

        float innerW = rect.width - pad * 2.0f;
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
        float y = rect.y + pad;
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
                float x = rect.x + pad + (cellW + gapX) * (float)col;
                float width = item->rect.width;
                float height = item->rect.height;
                item->layout(Rect(floorf(x + 0.5f), floorf(y + 0.5f), width, height));
                if (cellHeight <= 0.0f && item->rect.height > rowH)
                {
                    rowH = item->rect.height;
                }
            }
            y += rowH;
        }

        if (rect.width <= 0.0f)
        {
            float gaps = cols > 1 ? gapX * (float)(cols - 1) : 0.0f;
            this->rect.width = pad * 2.0f + cellW * (float)cols + gaps;
        }
        if (rect.height <= 0.0f)
        {
            this->rect.height = (y - rect.y) + pad;
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
            if (child->rect.width > maxW)
            {
                maxW = child->rect.width;
            }
        }

        float cellW = cellWidth;
        bool writeWidth = cellW > 0.0f || rect.width <= 0.0f;
        if (cellW <= 0.0f)
        {
            float innerW = rect.width - pad * 2.0f;
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
                    if (items[index]->rect.height > rowH)
                    {
                        rowH = items[index]->rect.height;
                    }
                }
            }
            contentH += rowH;
        }
        contentH += pad;
        rect.height = contentH;

        if (writeWidth)
        {
            float gaps = cols > 1 ? gapX * (float)(cols - 1) : 0.0f;
            rect.width = pad * 2.0f + cellW * (float)cols + gaps;
        }
    }

    void UIGrid::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        if (color.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), color);
        }
        UIWidget::render(shape);
    }
}
