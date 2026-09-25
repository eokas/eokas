#ifndef _EOKAS_UI_TABLE_H_
#define _EOKAS_UI_TABLE_H_

#include "../UIWidget.h"
#include <memory>
#include <optional>

namespace eokas
{
    struct UITableEdge
    {
        float thickness = 1.0f;
        Color color { 0.55f, 0.58f, 0.66f, 1.0f };
    };

    struct UITableBorder
    {
        UITableEdge left;
        UITableEdge top;
        UITableEdge right;
        UITableEdge bottom;

        UITableBorder() = default;
        UITableBorder(float thickness, const Color& color)
        {
            left.thickness = top.thickness = right.thickness = bottom.thickness = thickness;
            left.color = top.color = right.color = bottom.color = color;
        }
        UITableBorder(float leftThickness, float topThickness, float rightThickness, float bottomThickness, const Color& color)
        {
            left.thickness = leftThickness;
            top.thickness = topThickness;
            right.thickness = rightThickness;
            bottom.thickness = bottomThickness;
            left.color = top.color = right.color = bottom.color = color;
        }
    };

    class UITable;
    class UITableRow;

    class UITableCell : public UIWidget
    {
    public:
        UITableCell(UITable* table, UITableRow* row);
        void setContent(const std::shared_ptr<UIWidget>& widget);
        const std::shared_ptr<UIWidget>& content() const { return mContent; }
        void setBorder(const UITableBorder& value);
        void clearBorder();
        UITableBorder border() const;
        void layout(const Rect& rect) override;
        void render(UIShape& shape) override;

    private:
        int column() const;
        float shift() const;

        UITable* mTable = nullptr;
        UITableRow* mRow = nullptr;
        std::shared_ptr<UIWidget> mContent;
        std::optional<UITableBorder> mBorder;
    };

    class UITableRow : public UIWidget
    {
    public:
        UITableRow(UITable* table, UITableRow* parent, float height);
        UITable* table() const { return mTable; }
        UITableRow* parent() const { return mParent; }
        int index() const;
        int depth() const;

        int rowCount() const { return (int)mRows.size(); }
        UITableRow* row(int index) const;
        UITableRow* addRow(float height = 0.0f);
        UITableRow* insertRow(int index, float height = 0.0f);
        void removeRow(int index);

        float height() const { return mHeight; }
        void setHeight(float height);

        bool expanded() const { return mExpanded; }
        void setExpanded(bool value);
        std::function<void(bool)> onExpandedChanged;

        UITableCell* cell(int column) const;
        int columnOf(const UITableCell* cell) const;
        void setContent(int column, const std::shared_ptr<UIWidget>& widget);
        const std::shared_ptr<UIWidget>& getContent(int column) const;
        void insertCell(int index);
        void removeCell(int index);
        void collect(bool visibleOnly, std::vector<std::shared_ptr<UITableRow>>& out) const;

        void triggerClick() override;

    private:
        void syncCells();

        UITable* mTable = nullptr;
        UITableRow* mParent = nullptr;
        float mHeight = 0.0f;
        bool mExpanded = true;
        std::vector<std::shared_ptr<UITableRow>> mRows;
        std::vector<std::shared_ptr<UITableCell>> mCells;
    };

    class UITable : public UIWidget
    {
    public:
        UITableBorder border;
        float cellSpacing = 0.0f;
        float cellPadding = 0.0f;
        Color disclosure { 0.75f, 0.78f, 0.84f, 1.0f };

        UITable();
        int columnCount() const { return (int)mColumnWidths.size(); }
        void addColumn(float width = 0.0f);
        void insertColumn(int index, float width = 0.0f);
        void removeColumn(int index);
        float columnWidth(int index) const;
        void setColumnWidth(int index, float width);
        float columnIndent(int index) const;
        void setColumnIndent(int index, float size);

        int rowCount() const { return (int)mRows.size(); }
        UITableRow* row(int index) const;
        UITableRow* addRow(float height = 0.0f);
        UITableRow* insertRow(int index, float height = 0.0f);
        void removeRow(int index);
        float rowHeight(int index) const;
        void setRowHeight(int index, float height);

        UITableCell* cell(int row, int column) const;
        void setContent(int row, int column, const std::shared_ptr<UIWidget>& widget);
        const std::shared_ptr<UIWidget>& getContent(int row, int column) const;

        void layout(const Rect& rect) override;
        void refit();
        void render(UIShape& shape) override;

    private:
        std::vector<float> mColumnWidths;
        std::vector<float> mColumnIndents;
        std::vector<std::shared_ptr<UITableRow>> mRows;

        void appendRows(const std::shared_ptr<UITableRow>& row, bool visibleOnly, std::vector<std::shared_ptr<UITableRow>>& out) const;
        void syncChildren();
        float padding() const;
        float spacing() const;
        void axisInsets(const UITableCell* item, bool horizontal, float& leading, float& trailing) const;
        float contentSpan(const UITableRow* row, int column, bool horizontal) const;
        float columnPreferred(int index) const;
        float rowPreferred(const UITableRow* row) const;
        void resolveTracks(const std::vector<float>& given, const std::vector<float>& preferred, float span, std::vector<float>& sizes, float& used) const;
        void resolveColumns(float span, std::vector<float>& sizes, float& used) const;
    };
}

#endif//_EOKAS_UI_TABLE_H_
