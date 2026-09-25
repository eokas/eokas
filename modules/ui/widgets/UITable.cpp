#include "UITable.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    namespace
    {
        float snap(float v)
        {
            return floorf(v + 0.5f);
        }

        float positive(float v)
        {
            return v > 0.0f ? v : 0.0f;
        }

        void fitPair(float& a, float& b, float span)
        {
            a = positive(a);
            b = positive(b);
            float sum = a + b;
            if (sum > span && sum > 0.0f)
            {
                a = span * (a / sum);
                b = span * (b / sum);
            }
        }

        void rawInsets(const UITableBorder& border, float& left, float& top, float& right, float& bottom)
        {
            left = positive(border.left.thickness);
            top = positive(border.top.thickness);
            right = positive(border.right.thickness);
            bottom = positive(border.bottom.thickness);
        }

        void fittedInsets(const Rect& area, const UITableBorder& border, float& left, float& top, float& right, float& bottom)
        {
            rawInsets(border, left, top, right, bottom);
            fitPair(left, right, area.width);
            fitPair(top, bottom, area.height);
        }

        void drawEdge(UIShape& shape, const Rect& area, const Color& color)
        {
            if (area.width <= 0.0f || area.height <= 0.0f || color.a <= 0.0f)
            {
                return;
            }
            shape.addQuad(area, UIFont::solidUV(), color);
        }

        void drawBorder(UIShape& shape, const Rect& area, const UITableBorder& border)
        {
            float left = 0.0f;
            float top = 0.0f;
            float right = 0.0f;
            float bottom = 0.0f;
            fittedInsets(area, border, left, top, right, bottom);
            float midH = area.height - top - bottom;
            if (midH < 0.0f)
            {
                midH = 0.0f;
            }
            drawEdge(shape, Rect(area.x, area.y, area.width, top), border.top.color);
            drawEdge(shape, Rect(area.x, area.y + area.height - bottom, area.width, bottom), border.bottom.color);
            drawEdge(shape, Rect(area.x, area.y + top, left, midH), border.left.color);
            drawEdge(shape, Rect(area.x + area.width - right, area.y + top, right, midH), border.right.color);
        }

        void drawDisclosure(UIShape& shape, const Rect& slot, bool expanded, const Color& color)
        {
            float limit = slot.width < slot.height ? slot.width : slot.height;
            float arm = limit * 0.38f;
            if (arm < 1.0f || color.a <= 0.0f)
            {
                return;
            }
            float cx = slot.x + slot.width * 0.5f;
            float cy = slot.y + slot.height * 0.5f;
            Rect uv = UIFont::solidUV();
            if (expanded)
            {
                shape.addQuad(
                    Vector2(cx - arm, cy - arm * 0.45f),
                    Vector2(cx + arm, cy - arm * 0.45f),
                    Vector2(cx, cy + arm * 0.70f),
                    Vector2(cx, cy + arm * 0.70f),
                    uv, color);
            }
            else
            {
                shape.addQuad(
                    Vector2(cx - arm * 0.45f, cy - arm),
                    Vector2(cx - arm * 0.45f, cy + arm),
                    Vector2(cx + arm * 0.70f, cy),
                    Vector2(cx + arm * 0.70f, cy),
                    uv, color);
            }
        }

        const std::shared_ptr<UIWidget>& emptyWidget()
        {
            static const std::shared_ptr<UIWidget> kEmpty;
            return kEmpty;
        }

        float contentShift(const UITable* table, const UITableRow* row, int column)
        {
            if (table == nullptr || row == nullptr)
            {
                return 0.0f;
            }
            float indent = table->columnIndent(column);
            if (indent <= 0.0f)
            {
                return 0.0f;
            }
            return (float)(row->depth() + 1) * indent;
        }

        void relayout(UITable* table)
        {
            if (table == nullptr)
            {
                return;
            }
            Rect next = table->rect;
            next.height = 0.0f;
            table->layout(next);
        }
    }

    UITableCell::UITableCell(UITable* table, UITableRow* row)
        : mTable(table)
        , mRow(row)
    {
        interactive = false;
        color = Color(0.0f, 0.0f, 0.0f, 0.0f);
    }

    void UITableCell::setContent(const std::shared_ptr<UIWidget>& widget)
    {
        mContent = widget;
        children.clear();
        if (mContent)
        {
            children.push_back(mContent);
        }
    }

    void UITableCell::setBorder(const UITableBorder& value)
    {
        mBorder = value;
    }

    void UITableCell::clearBorder()
    {
        mBorder.reset();
    }

    UITableBorder UITableCell::border() const
    {
        if (mBorder.has_value())
        {
            return *mBorder;
        }
        if (mTable != nullptr)
        {
            return mTable->border;
        }
        return UITableBorder();
    }

    int UITableCell::column() const
    {
        if (mRow == nullptr)
        {
            return -1;
        }
        return mRow->columnOf(this);
    }

    float UITableCell::shift() const
    {
        return contentShift(mTable, mRow, this->column());
    }

    void UITableCell::layout(const Rect& rect)
    {
        this->rect = rect;
        if (!mContent)
        {
            return;
        }
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
        fittedInsets(rect, this->border(), left, top, right, bottom);
        float pad = mTable != nullptr ? mTable->cellPadding : 0.0f;
        if (pad < 0.0f)
        {
            pad = 0.0f;
        }
        float x = rect.x + left + pad + this->shift();
        float y = rect.y + top + pad;
        mContent->layout(Rect(snap(x), snap(y), mContent->rect.width, mContent->rect.height));
    }

    void UITableCell::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        if (color.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), color);
        }
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
        fittedInsets(rect, this->border(), left, top, right, bottom);
        float pad = mTable != nullptr ? mTable->cellPadding : 0.0f;
        if (pad < 0.0f)
        {
            pad = 0.0f;
        }
        float shift = this->shift();
        if (shift > 0.0f && mRow != nullptr && mRow->rowCount() > 0 && mTable != nullptr)
        {
            int depth = mRow->depth();
            float indent = shift / (float)(depth + 1);
            float slotY = rect.y + top;
            float slotH = rect.height - top - bottom;
            if (slotH < 0.0f)
            {
                slotH = 0.0f;
            }
            drawDisclosure(shape, Rect(rect.x + left + pad + (float)depth * indent, slotY, indent, slotH), mRow->expanded(), mTable->disclosure);
        }
        float x = rect.x + left + pad + shift;
        float y = rect.y + top + pad;
        float w = rect.width - left - right - pad * 2.0f - shift;
        float h = rect.height - top - bottom - pad * 2.0f;
        if (w < 0.0f)
        {
            w = 0.0f;
        }
        if (h < 0.0f)
        {
            h = 0.0f;
        }
        Vector2 origin = shape.offset();
        shape.pushClip(Rect(origin.x + x, origin.y + y, w, h));
        UIWidget::render(shape);
        shape.popClip();
        drawBorder(shape, rect, this->border());
    }

    UITableRow::UITableRow(UITable* table, UITableRow* parent, float height)
        : mTable(table)
        , mParent(parent)
        , mHeight(positive(height))
    {
        interactive = true;
        color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        int count = table != nullptr ? table->columnCount() : 0;
        mCells.reserve((size_t)count);
        for (int i = 0; i < count; ++i)
        {
            mCells.push_back(std::make_shared<UITableCell>(table, this));
        }
        this->syncCells();
    }

    void UITableRow::syncCells()
    {
        children.clear();
        for (auto& item : mCells)
        {
            if (item)
            {
                children.push_back(item);
            }
        }
    }

    int UITableRow::index() const
    {
        UITableRow* owner = mParent;
        int count = owner != nullptr ? owner->rowCount() : (mTable != nullptr ? mTable->rowCount() : 0);
        for (int i = 0; i < count; ++i)
        {
            UITableRow* item = owner != nullptr ? owner->row(i) : mTable->row(i);
            if (item == this)
            {
                return i;
            }
        }
        return -1;
    }

    int UITableRow::depth() const
    {
        int value = 0;
        for (UITableRow* node = mParent; node != nullptr; node = node->mParent)
        {
            value += 1;
        }
        return value;
    }

    UITableRow* UITableRow::row(int index) const
    {
        if (index < 0 || index >= this->rowCount())
        {
            return nullptr;
        }
        return mRows[(size_t)index].get();
    }

    UITableRow* UITableRow::addRow(float height)
    {
        return this->insertRow(this->rowCount(), height);
    }

    UITableRow* UITableRow::insertRow(int index, float height)
    {
        if (mTable == nullptr || index < 0 || index > this->rowCount())
        {
            return nullptr;
        }
        auto child = std::make_shared<UITableRow>(mTable, this, height);
        mRows.insert(mRows.begin() + index, child);
        relayout(mTable);
        return child.get();
    }

    void UITableRow::removeRow(int index)
    {
        if (index < 0 || index >= this->rowCount())
        {
            return;
        }
        mRows.erase(mRows.begin() + index);
        relayout(mTable);
    }

    void UITableRow::setHeight(float height)
    {
        mHeight = positive(height);
    }

    void UITableRow::setExpanded(bool value)
    {
        if (mExpanded == value)
        {
            return;
        }
        mExpanded = value;
        if (mTable != nullptr)
        {
            relayout(mTable);
        }
        if (onExpandedChanged)
        {
            onExpandedChanged(mExpanded);
        }
    }

    UITableCell* UITableRow::cell(int column) const
    {
        if (column < 0 || column >= (int)mCells.size())
        {
            return nullptr;
        }
        return mCells[(size_t)column].get();
    }

    int UITableRow::columnOf(const UITableCell* cell) const
    {
        int count = (int)mCells.size();
        for (int i = 0; i < count; ++i)
        {
            if (mCells[(size_t)i].get() == cell)
            {
                return i;
            }
        }
        return -1;
    }

    void UITableRow::insertCell(int index)
    {
        if (index < 0 || index > (int)mCells.size())
        {
            return;
        }
        mCells.insert(mCells.begin() + index, std::make_shared<UITableCell>(mTable, this));
        this->syncCells();
        for (const auto& child : mRows)
        {
            if (child)
            {
                child->insertCell(index);
            }
        }
    }

    void UITableRow::removeCell(int index)
    {
        if (index < 0 || index >= (int)mCells.size())
        {
            return;
        }
        mCells.erase(mCells.begin() + index);
        this->syncCells();
        for (const auto& child : mRows)
        {
            if (child)
            {
                child->removeCell(index);
            }
        }
    }

    void UITableRow::collect(bool visibleOnly, std::vector<std::shared_ptr<UITableRow>>& out) const
    {
        if (visibleOnly && !mExpanded)
        {
            return;
        }
        for (const auto& child : mRows)
        {
            if (!child)
            {
                continue;
            }
            out.push_back(child);
            child->collect(visibleOnly, out);
        }
    }

    void UITableRow::setContent(int column, const std::shared_ptr<UIWidget>& widget)
    {
        UITableCell* item = this->cell(column);
        if (item == nullptr)
        {
            return;
        }
        item->setContent(widget);
    }

    const std::shared_ptr<UIWidget>& UITableRow::getContent(int column) const
    {
        UITableCell* item = this->cell(column);
        if (item == nullptr)
        {
            return emptyWidget();
        }
        return item->content();
    }

    void UITableRow::triggerClick()
    {
        if (this->rowCount() > 0)
        {
            this->setExpanded(!this->expanded());
        }
        UIWidget::triggerClick();
    }

    UITable::UITable()
    {
        interactive = false;
    }

    void UITable::appendRows(const std::shared_ptr<UITableRow>& row, bool visibleOnly, std::vector<std::shared_ptr<UITableRow>>& out) const
    {
        if (!row)
        {
            return;
        }
        out.push_back(row);
        row->collect(visibleOnly, out);
    }

    void UITable::syncChildren()
    {
        children.clear();
        std::vector<std::shared_ptr<UITableRow>> rows;
        for (const auto& row : mRows)
        {
            this->appendRows(row, true, rows);
        }
        for (const auto& row : rows)
        {
            children.push_back(row);
        }
    }

    float UITable::padding() const
    {
        return positive(cellPadding);
    }

    float UITable::spacing() const
    {
        return positive(cellSpacing);
    }

    void UITable::addColumn(float width)
    {
        this->insertColumn(this->columnCount(), width);
    }

    void UITable::insertColumn(int index, float width)
    {
        if (index < 0 || index > this->columnCount())
        {
            return;
        }
        mColumnWidths.insert(mColumnWidths.begin() + index, positive(width));
        mColumnIndents.insert(mColumnIndents.begin() + index, 0.0f);
        for (const auto& row : mRows)
        {
            if (row)
            {
                row->insertCell(index);
            }
        }
        relayout(this);
    }

    void UITable::removeColumn(int index)
    {
        if (index < 0 || index >= this->columnCount())
        {
            return;
        }
        mColumnWidths.erase(mColumnWidths.begin() + index);
        mColumnIndents.erase(mColumnIndents.begin() + index);
        for (const auto& row : mRows)
        {
            if (row)
            {
                row->removeCell(index);
            }
        }
        relayout(this);
    }

    float UITable::columnWidth(int index) const
    {
        if (index < 0 || index >= this->columnCount())
        {
            return 0.0f;
        }
        return mColumnWidths[(size_t)index];
    }

    void UITable::setColumnWidth(int index, float width)
    {
        if (index < 0 || index >= this->columnCount())
        {
            return;
        }
        mColumnWidths[(size_t)index] = positive(width);
    }

    float UITable::columnIndent(int index) const
    {
        if (index < 0 || index >= this->columnCount())
        {
            return 0.0f;
        }
        return mColumnIndents[(size_t)index];
    }

    void UITable::setColumnIndent(int index, float size)
    {
        if (index < 0 || index >= this->columnCount())
        {
            return;
        }
        mColumnIndents[(size_t)index] = positive(size);
    }

    UITableRow* UITable::row(int index) const
    {
        if (index < 0 || index >= this->rowCount())
        {
            return nullptr;
        }
        return mRows[(size_t)index].get();
    }

    UITableRow* UITable::addRow(float height)
    {
        return this->insertRow(this->rowCount(), height);
    }

    UITableRow* UITable::insertRow(int index, float height)
    {
        if (index < 0 || index > this->rowCount())
        {
            return nullptr;
        }
        auto child = std::make_shared<UITableRow>(this, nullptr, height);
        mRows.insert(mRows.begin() + index, child);
        relayout(this);
        return child.get();
    }

    void UITable::removeRow(int index)
    {
        if (index < 0 || index >= this->rowCount())
        {
            return;
        }
        mRows.erase(mRows.begin() + index);
        relayout(this);
    }

    float UITable::rowHeight(int index) const
    {
        UITableRow* line = this->row(index);
        if (line == nullptr)
        {
            return 0.0f;
        }
        return line->height();
    }

    void UITable::setRowHeight(int index, float height)
    {
        if (UITableRow* line = this->row(index))
        {
            line->setHeight(height);
        }
    }

    UITableCell* UITable::cell(int row, int column) const
    {
        UITableRow* line = this->row(row);
        if (line == nullptr)
        {
            return nullptr;
        }
        return line->cell(column);
    }

    void UITable::setContent(int row, int column, const std::shared_ptr<UIWidget>& widget)
    {
        UITableCell* item = this->cell(row, column);
        if (item == nullptr)
        {
            return;
        }
        item->setContent(widget);
    }

    const std::shared_ptr<UIWidget>& UITable::getContent(int row, int column) const
    {
        UITableCell* item = this->cell(row, column);
        if (item == nullptr)
        {
            return emptyWidget();
        }
        return item->content();
    }

    void UITable::axisInsets(const UITableCell* item, bool horizontal, float& leading, float& trailing) const
    {
        UITableBorder edges = item != nullptr ? item->border() : border;
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
        rawInsets(edges, left, top, right, bottom);
        if (horizontal)
        {
            leading = left;
            trailing = right;
        }
        else
        {
            leading = top;
            trailing = bottom;
        }
    }

    float UITable::contentSpan(const UITableRow* row, int column, bool horizontal) const
    {
        UITableCell* item = row != nullptr ? row->cell(column) : nullptr;
        if (item == nullptr)
        {
            return 0.0f;
        }
        const std::shared_ptr<UIWidget>& body = item->content();
        if (!body || !body->visible)
        {
            return 0.0f;
        }
        return horizontal ? body->rect.width : body->rect.height;
    }

    float UITable::columnPreferred(int index) const
    {
        float given = mColumnWidths[(size_t)index];
        if (given > 0.0f)
        {
            return given;
        }
        float pad = this->padding();
        float best = 0.0f;
        std::vector<std::shared_ptr<UITableRow>> rows;
        for (const auto& row : mRows)
        {
            this->appendRows(row, false, rows);
        }
        for (const auto& row : rows)
        {
            float leading = 0.0f;
            float trailing = 0.0f;
            this->axisInsets(row->cell(index), true, leading, trailing);
            float span = leading + trailing + pad * 2.0f + this->contentSpan(row.get(), index, true) + contentShift(this, row.get(), index);
            if (span > best)
            {
                best = span;
            }
        }
        return best;
    }

    float UITable::rowPreferred(const UITableRow* row) const
    {
        if (row == nullptr)
        {
            return 0.0f;
        }
        if (row->height() > 0.0f)
        {
            return row->height();
        }
        float pad = this->padding();
        float best = 0.0f;
        int columns = this->columnCount();
        for (int column = 0; column < columns; ++column)
        {
            float leading = 0.0f;
            float trailing = 0.0f;
            this->axisInsets(row->cell(column), false, leading, trailing);
            float span = leading + trailing + pad * 2.0f + this->contentSpan(row, column, false);
            if (span > best)
            {
                best = span;
            }
        }
        return best;
    }

    void UITable::resolveTracks(const std::vector<float>& given, const std::vector<float>& preferred, float span, std::vector<float>& sizes, float& used) const
    {
        int count = (int)given.size();
        float prefSum = count > 1 ? this->spacing() * (float)(count - 1) : 0.0f;
        int autos = 0;
        float fixed = 0.0f;
        float autoSum = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            prefSum += preferred[(size_t)i];
            if (given[(size_t)i] > 0.0f)
            {
                fixed += preferred[(size_t)i];
            }
            else
            {
                autos += 1;
                autoSum += preferred[(size_t)i];
            }
        }
        sizes.assign((size_t)count, 0.0f);
        if (span <= 0.0f || span >= prefSum)
        {
            float extra = span <= 0.0f ? 0.0f : span - prefSum;
            float add = autos > 0 ? extra / (float)autos : 0.0f;
            for (int i = 0; i < count; ++i)
            {
                sizes[(size_t)i] = preferred[(size_t)i];
                if (!(given[(size_t)i] > 0.0f))
                {
                    sizes[(size_t)i] += add;
                }
            }
            used = span <= 0.0f ? prefSum : span;
            return;
        }
        float room = span - (count > 1 ? this->spacing() * (float)(count - 1) : 0.0f) - fixed;
        if (room < 0.0f)
        {
            room = 0.0f;
        }
        float scale = autoSum > 0.0f ? room / autoSum : 0.0f;
        for (int i = 0; i < count; ++i)
        {
            if (given[(size_t)i] > 0.0f)
            {
                sizes[(size_t)i] = preferred[(size_t)i];
            }
            else
            {
                sizes[(size_t)i] = preferred[(size_t)i] * scale;
            }
        }
        used = span;
    }

    void UITable::resolveColumns(float span, std::vector<float>& sizes, float& used) const
    {
        int count = this->columnCount();
        std::vector<float> preferred((size_t)count, 0.0f);
        for (int i = 0; i < count; ++i)
        {
            preferred[(size_t)i] = this->columnPreferred(i);
        }
        this->resolveTracks(mColumnWidths, preferred, span, sizes, used);
    }

    void UITable::layout(const Rect& rect)
    {
        this->rect = rect;
        this->syncChildren();
        std::vector<float> widths;
        float usedW = 0.0f;
        this->resolveColumns(rect.width, widths, usedW);
        std::vector<std::shared_ptr<UITableRow>> lines;
        for (const auto& row : mRows)
        {
            this->appendRows(row, true, lines);
        }
        std::vector<float> given((size_t)lines.size(), 0.0f);
        std::vector<float> preferred((size_t)lines.size(), 0.0f);
        for (int i = 0; i < (int)lines.size(); ++i)
        {
            given[(size_t)i] = lines[(size_t)i]->height();
            preferred[(size_t)i] = this->rowPreferred(lines[(size_t)i].get());
        }
        std::vector<float> heights;
        float usedH = 0.0f;
        this->resolveTracks(given, preferred, rect.height, heights, usedH);
        if (rect.width <= 0.0f)
        {
            this->rect.width = usedW;
        }
        if (rect.height <= 0.0f)
        {
            this->rect.height = usedH;
        }
        float gap = this->spacing();
        int columns = this->columnCount();
        float rowW = 0.0f;
        for (int column = 0; column < columns; ++column)
        {
            rowW += widths[(size_t)column];
        }
        if (columns > 1)
        {
            rowW += gap * (float)(columns - 1);
        }
        float y = rect.y;
        for (int i = 0; i < (int)lines.size(); ++i)
        {
            if (i > 0)
            {
                y += gap;
            }
            float rowH = heights[(size_t)i];
            lines[(size_t)i]->rect = Rect(snap(rect.x), snap(y), rowW, rowH);
            float x = rect.x;
            for (int column = 0; column < columns; ++column)
            {
                if (column > 0)
                {
                    x += gap;
                }
                float colW = widths[(size_t)column];
                if (UITableCell* item = lines[(size_t)i]->cell(column))
                {
                    item->layout(Rect(snap(x), snap(y), colW, rowH));
                }
                x += colW;
            }
            y += rowH;
        }
    }

    void UITable::refit()
    {
        Rect next = rect;
        next.width = 0.0f;
        next.height = 0.0f;
        this->layout(next);
    }

    void UITable::render(UIShape& shape)
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
