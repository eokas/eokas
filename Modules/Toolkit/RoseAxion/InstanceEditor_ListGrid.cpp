#include "./InstanceEditor_ListGrid.h"
#include "./FieldEditors.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"
#include "UI/widgets/UITable.h"

#include <cmath>
#include <functional>
#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    namespace {

        int clampi(int value, int lo, int hi)
        {
            if (value < lo) return lo;
            if (value > hi) return hi;
            return value;
        }

        void revealCell(UIView& view, int row, int col, float headW, const std::vector<float>& colWs, float rowH)
        {
            if (row < 0 || col < 0 || col >= static_cast<int>(colWs.size()))
            {
                return;
            }
            float x = headW;
            for (int i = 0; i < col; ++i)
            {
                x += colWs[static_cast<size_t>(i)];
            }
            float colW = colWs[static_cast<size_t>(col)];
            float y = rowH + static_cast<float>(row) * rowH;
            Rect vp = view.viewport();
            float dx = 0.0f;
            float dy = 0.0f;
            if (x < view.scrollX())
            {
                dx = x - view.scrollX();
            }
            else if (x + colW > view.scrollX() + vp.size.x)
            {
                dx = x + colW - (view.scrollX() + vp.size.x);
            }
            if (y < view.scrollY())
            {
                dy = y - view.scrollY();
            }
            else if (y + rowH > view.scrollY() + vp.size.y)
            {
                dy = y + rowH - (view.scrollY() + vp.size.y);
            }
            if (dx != 0.0f || dy != 0.0f)
            {
                view.scrollBy(dx, dy);
            }
        }

    }

    void RoseInstanceEditor_ListGrid::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseInstanceEditor_ListGrid::clearOverlayEditors()
    {
        UIWidget* overlay = mOverlay != nullptr ? mOverlay : mHost.overlay;
        if (overlay != nullptr)
        {
            if (mEditInput != nullptr)
            {
                overlay->detachChild(mEditInput);
            }
            if (mPickMenu != nullptr)
            {
                overlay->detachChild(mPickMenu);
            }
            if (mDatePanel != nullptr)
            {
                overlay->detachChild(mDatePanel);
            }
            if (mColorPanel != nullptr)
            {
                overlay->detachChild(mColorPanel);
            }
        }
        mEditInput = nullptr;
        mPickMenu = nullptr;
        mDatePanel = nullptr;
        mColorPanel = nullptr;
        mColorPicker = nullptr;
        mEditAnchor = nullptr;
        mPickAnchor = nullptr;
        mDateAnchor = nullptr;
        mColorAnchor = nullptr;
    }

    void RoseInstanceEditor_ListGrid::setOverlay(UIWidget* overlay)
    {
        if (mOverlay == overlay)
        {
            return;
        }
        this->clearOverlayEditors();
        mOverlay = overlay;
        mHost.overlay = overlay;
    }

    bool RoseInstanceEditor_ListGrid::hasPopupEdit() const
    {
        return mEdit != EditKind::None || mPickRow >= 0 || mDateRow >= 0 || mColorRow >= 0;
    }

    const String& RoseInstanceEditor_ListGrid::font() const
    {
        return mHost.font;
    }

    float RoseInstanceEditor_ListGrid::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseInstanceEditor_ListGrid::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseInstanceEditor_ListGrid::refreshUi()
    {
        this->notifyRebuild();
    }

    void RoseInstanceEditor_ListGrid::reset()
    {
        mSelect = SelectKind::Cell;
        mSelectRow = 0;
        mSelectCol = 0;
        mEdit = EditKind::None;
        mClickRowHead = false;
        mDateRow = -1;
        mDateCol = -1;
        mPickRow = -1;
        mPickCol = -1;
        mColorRow = -1;
        mColorCol = -1;
        mClickHeader = false;
        mClickRow = -1;
        mClickCol = -1;
        mKeepX = 0.0f;
        mKeepY = 0.0f;
        this->setOverlay(nullptr);
        mView = nullptr;
        mTable = nullptr;
        mGridHost = nullptr;
        mEditAnchor = nullptr;
        mPickAnchor = nullptr;
        mDateAnchor = nullptr;
        mColorAnchor = nullptr;
        mFocus = nullptr;
        this->detachChildren();
    }

    float RoseInstanceEditor_ListGrid::scrollX() const
    {
        return mView != nullptr ? mView->scrollX() : mKeepX;
    }

    float RoseInstanceEditor_ListGrid::scrollY() const
    {
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseInstanceEditor_ListGrid::setScroll(float x, float y)
    {
        mKeepX = x;
        mKeepY = y;
        if (mView != nullptr)
        {
            mView->setScroll(x, y);
        }
    }

    void RoseInstanceEditor_ListGrid::layout()
    {
        if (!visible)
        {
            return;
        }
        if (mView != nullptr)
        {
            float viewH = shape.size.y - mBarH;
            if (viewH < 1.0f)
            {
                viewH = 1.0f;
            }
            setBox(*mView, Rect(0.0f, mBarH, shape.size.x, viewH));
            mView->layout();
        }
        this->placeOverlayEditors();
        UIWidget::layout();
    }

    void RoseInstanceEditor_ListGrid::placeOverlayEditors()
    {
        if (mOverlay == nullptr)
        {
            return;
        }
        if (mEditInput != nullptr && mEditAnchor != nullptr)
        {
            placeOnOverlay(*mOverlay, *mEditInput, mEditAnchor->screenBounds(), false);
        }
        if (mPickMenu != nullptr && mPickAnchor != nullptr)
        {
            placeOnOverlay(*mOverlay, *mPickMenu, mPickAnchor->screenBounds(), true);
        }
        if (mDatePanel != nullptr && mDateAnchor != nullptr)
        {
            placeOnOverlay(*mOverlay, *mDatePanel, mDateAnchor->screenBounds(), true);
        }
        if (mColorPanel != nullptr && mColorAnchor != nullptr)
        {
            placeOnOverlay(*mOverlay, *mColorPanel, mColorAnchor->screenBounds(), true);
        }
    }


    bool RoseInstanceEditor_ListGrid::isReadOnlyColumn(u32_t col) const
    {
        if (mDoc == nullptr)
        {
            return true;
        }
        NestedField info = mDoc->instanceListColumn(col);
        return info.nestedStruct || info.nestedList;
    }

    void RoseInstanceEditor_ListGrid::clampState()
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        int rows = static_cast<int>(mDoc->instanceListCount());
        int cols = static_cast<int>(mDoc->instanceListColumnCount());
        if (rows > 0)
        {
            mSelectRow = clampi(mSelectRow, 0, rows - 1);
        }
        else
        {
            mSelectRow = 0;
        }
        if (cols > 0)
        {
            mSelectCol = clampi(mSelectCol, 0, cols - 1);
        }
        else
        {
            mSelectCol = 0;
        }
        if ((mSelect == SelectKind::Cell || mSelect == SelectKind::All) && (rows <= 0 || cols <= 0))
        {
            mSelect = SelectKind::None;
        }
        if (mSelect == SelectKind::Row && rows <= 0)
        {
            mSelect = SelectKind::None;
        }
        if (mSelect == SelectKind::Column && cols <= 0)
        {
            mSelect = SelectKind::None;
        }
        if (mSelect == SelectKind::None && rows > 0 && cols > 0)
        {
            mSelect = SelectKind::Cell;
        }
        if (mDateRow >= rows || mDateCol >= cols) { mDateRow = -1; mDateCol = -1; }
        else if (mDateCol >= 0 && cols > 0 && !temporalKind(mDoc->instanceListColumn(static_cast<u32_t>(mDateCol)).kind)) { mDateRow = -1; mDateCol = -1; }
        if (mPickRow >= rows || mPickCol >= cols) { mPickRow = -1; mPickCol = -1; }
        if (mColorRow >= rows || mColorCol >= cols) { mColorRow = -1; mColorCol = -1; }
        else if (mColorCol >= 0 && cols > 0 && mDoc->instanceListColumn(static_cast<u32_t>(mColorCol)).kind != FieldKind::Color) { mColorRow = -1; mColorCol = -1; }
        if (mEdit == EditKind::Cell && (rows <= 0 || cols <= 0 || mEditRow >= rows || mEditCol >= cols)) mEdit = EditKind::None;
        if (mEdit == EditKind::Cell && cols > 0 && pickerKind(mDoc->instanceListColumn(static_cast<u32_t>(mEditCol)).kind)) mEdit = EditKind::None;
    }

    void RoseInstanceEditor_ListGrid::finishEdit(const String& text, int rowDelta, int colDelta)
    {
        if (mEdit == EditKind::None || mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        {
            mDoc->setInstanceListCellText(static_cast<u32_t>(mEditRow), static_cast<u32_t>(mEditCol), text);
            int rowHi = static_cast<int>(mDoc->instanceListCount()) - 1;
            int colHi = static_cast<int>(mDoc->instanceListColumnCount()) - 1;
            if (rowHi < 0 || colHi < 0)
            {
                mSelect = SelectKind::None;
                mSelectRow = 0;
                mSelectCol = 0;
            }
            else
            {
                mSelect = SelectKind::Cell;
                mSelectRow = clampi(mSelectRow + rowDelta, 0, rowHi);
                mSelectCol = clampi(mSelectCol + colDelta, 0, colHi);
            }
        }
        mEdit = EditKind::None;
        this->refreshUi();
    }

    void RoseInstanceEditor_ListGrid::beginEdit(int row, int col, const String& seed)
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        if (this->isReadOnlyColumn(static_cast<u32_t>(col)))
        {
            return;
        }
        FieldKind kind = mDoc->instanceListColumn(static_cast<u32_t>(col)).kind;
        mSelect = SelectKind::Cell;
        mSelectRow = row;
        mSelectCol = col;
        mEdit = EditKind::None;
        if (temporalKind(kind))
        {
            TemporalParts parts;
            initTemporalParts(kind, mDoc->instanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col)), parts);
            mDateYear = parts.year;
            mDateMonth = parts.month;
            mDateDay = parts.day;
            mDateHour = parts.hour;
            mDateMinute = parts.minute;
            mDateSecond = parts.second;
            mDateRow = row;
            mDateCol = col;
            mColorRow = -1;
            mColorCol = -1;
            mPickRow = -1;
            mPickCol = -1;
            this->refreshUi();
            return;
        }
        if (kind == FieldKind::Color)
        {
            if (mColorRow == row && mColorCol == col)
            {
                this->refreshUi();
                return;
            }
            mColorRow = row;
            mColorCol = col;
            mDateRow = -1;
            mDateCol = -1;
            mPickRow = -1;
            mPickCol = -1;
            this->refreshUi();
            return;
        }
        if (kind == FieldKind::Boolean)
        {
            return;
        }
        if (pathKind(kind))
        {
            mPickRow = -1;
            mPickCol = -1;
            mDateRow = -1;
            mDateCol = -1;
            mColorRow = -1;
            mColorCol = -1;
            String current = mDoc->instanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
            String picked = String::empty;
            if (kind == FieldKind::FilePath && mHost.pickFilePath)
            {
                picked = mHost.pickFilePath(current);
            }
            else if (kind == FieldKind::FolderPath && mHost.pickFolderPath)
            {
                picked = mHost.pickFolderPath(current);
            }
            if (!picked.isEmpty())
            {
                mDoc->setInstanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), picked);
            }
            this->refreshUi();
            return;
        }
        if (optionKind(kind))
        {
            mPickRow = row;
            mPickCol = col;
            mDateRow = -1;
            mDateCol = -1;
            mColorRow = -1;
            mColorCol = -1;
            this->refreshUi();
            return;
        }
        mEdit = EditKind::Cell;
        mEditRow = row;
        mEditCol = col;
        mEditSeed = seed;
        mPickRow = -1;
        mPickCol = -1;
        mDateRow = -1;
        mDateCol = -1;
        mColorRow = -1;
        mColorCol = -1;
        this->refreshUi();
    }

    void RoseInstanceEditor_ListGrid::selectCell(int row, int col)
    {
        auto now = std::chrono::steady_clock::now();
        bool repeat = !mClickHeader && mClickRow == row && mClickCol == col && now - mClickTime < std::chrono::milliseconds(450);
        mClickTime = now;
        mClickRow = row;
        mClickCol = col;
        mClickHeader = false;
        mClickRowHead = false;
        mSelect = SelectKind::Cell;
        mSelectRow = row;
        mSelectCol = col;
        if (repeat)
        {
            if (!this->isReadOnlyColumn(static_cast<u32_t>(col)))
            {
                this->beginEdit(row, col, mDoc->instanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col)));
            }
            return;
        }
        mPickRow = -1;
        mPickCol = -1;
        this->refreshUi();
    }


    void RoseInstanceEditor_ListGrid::setBoolCell(int row, int col, bool value)
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        mDoc->setInstanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), value ? "true" : "false");
        mSelect = SelectKind::Cell;
        mSelectRow = row;
        mSelectCol = col;
        mEdit = EditKind::None;
        mPickRow = -1;
        mPickCol = -1;
        this->refreshUi();
    }

    void RoseInstanceEditor_ListGrid::toggleBoolCell(int row, int col)
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        this->setBoolCell(row, col, !boolFromText(mDoc->instanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col))));
    }

    void RoseInstanceEditor_ListGrid::selectAll()
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        mSelect = SelectKind::All;
        mPickRow = -1;
        mPickCol = -1;
        mClickHeader = false;
        mClickRowHead = false;
        mClickRow = -1;
        mClickCol = -1;
        this->refreshUi();
    }

    bool RoseInstanceEditor_ListGrid::handleEscape()
    {
if (mDateRow >= 0 || mDateCol >= 0)
        {
            mDateRow = -1;
            mDateCol = -1;
            this->refreshUi();
            return true;
        }
        if (mColorRow >= 0 || mColorCol >= 0)
        {
            if (mColorPicker != nullptr && mColorPicker->eyeDropping())
            {
                mColorPicker->cancelEyeDropper();
                return true;
            }
            mColorRow = -1;
            mColorCol = -1;
            this->refreshUi();
            return true;
        }
        if (mEdit != EditKind::None)
        {
            mEdit = EditKind::None;
            this->refreshUi();
            return true;
        }
        if (mPickRow >= 0 || mPickCol >= 0)
        {
            mPickRow = -1;
            mPickCol = -1;
            this->refreshUi();
            return true;
        }
        return false;
    }

    bool RoseInstanceEditor_ListGrid::colorDropping() const
    {
        return mColorPicker != nullptr && mColorPicker->eyeDropping();
    }

    void RoseInstanceEditor_ListGrid::sampleColorDropper(float x, float y, bool commit)
    {
        if (mColorPicker != nullptr)
        {
            mColorPicker->sampleEyeDropper(x, y, commit);
        }
    }

    void RoseInstanceEditor_ListGrid::cancelColorDropper()
    {
        if (mColorPicker != nullptr)
        {
            mColorPicker->cancelEyeDropper();
        }
    }







    void RoseInstanceEditor_ListGrid::rebuild()
    {
        this->clearOverlayEditors();
        this->detachChildren();
        mView = nullptr;
        mTable = nullptr;
        mGridHost = nullptr;
        mFocus = nullptr;
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveInstance())
        {
            return;
        }
        this->clampState();

        const float s = this->scale();
        const float fontPx = 15.0f * s;
        const float smallPx = 13.0f * s;
        const float rowH = 36.0f * s;
        const RoseTheme& t = theme();
        const Color paper = t.paper;
        const Color headerBg = t.header;
        const Color selected = t.selected;
        const Color selectedCell = t.selectedCell;
        const Color ink = t.ink;
        const Color field = t.field;
        const Color popup = t.popup;
        const Color url = t.url;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const String& fontPath = this->font();
        this->shape.pivot = Vector2(0.0f, 0.0f);
        fillSurface(*this, paper);
        const float height = shape.size.y;
        mBarH = 0.0f;
        const float pad = 8.0f * s;
        float viewH = height;
        if (viewH < 1.0f) { viewH = 1.0f; }
        UIWidget* overlay = mOverlay != nullptr ? mOverlay : mHost.overlay;

        int rows = static_cast<int>(mDoc->instanceListCount());
        int cols = static_cast<int>(mDoc->instanceListColumnCount());
        const float headW = 56.0f * s;
        float minColW = 120.0f * s;
        float avail = shape.size.x - headW;
        if (avail < minColW && cols > 0) { avail = minColW * static_cast<float>(cols); }
        float colW = cols > 0 ? avail / static_cast<float>(cols) : minColW;
        if (colW < minColW) { colW = minColW; }
        std::vector<float> colWs(static_cast<size_t>(cols), colW);

        auto view = std::make_shared<UIView>();
        view->shape.pivot = Vector2(0.0f, 0.0f);
        fillSurface(*view, paper);
        setBox(view, Rect(0.0f, 0.0f, shape.size.x, viewH));

        auto table = std::make_shared<UITable>();
        table->shape.pivot = Vector2(0.0f, 0.0f);
        const Color gridLine = t.gridLine;
        table->border = UITableBorder(1.0f, 1.0f, 0.0f, 0.0f, gridLine);
        table->cellPadding = 0.0f;
        table->cellSpacing = 0.0f;
        table->color = paper;
        table->addColumn(headW);
        for (int col = 0; col < cols; ++col)
        {
            table->addColumn(colWs[static_cast<size_t>(col)]);
        }
        for (int row = 0; row < rows + 1; ++row)
        {
            table->addRow(rowH);
        }
        float tableW = headW;
        for (float w : colWs)
        {
            tableW += w;
        }
        float tableH = rowH * static_cast<float>(rows + 1);
        setBox(table, Rect(0.0f, 0.0f, tableW, tableH));

        auto paintCell = [&](UITableCell* cell, int tableRow, int tableCol, const Color& fill)
        {
            if (cell == nullptr)
            {
                return;
            }
            cell->color = fill;
            bool lastCol = tableCol == cols;
            bool lastRow = tableRow == rows;
            cell->setBorder(UITableBorder(1.0f, 1.0f, lastCol ? 1.0f : 0.0f, lastRow ? 1.0f : 0.0f, gridLine));
        };
        auto innerBox = [&](int tableRow, int tableCol, float spanW, float spanH)
        {
            bool lastCol = tableCol == cols;
            bool lastRow = tableRow == rows;
            float w = spanW - 1.0f - (lastCol ? 1.0f : 0.0f);
            float h = spanH - 1.0f - (lastRow ? 1.0f : 0.0f);
            if (w < 0.0f) w = 0.0f;
            if (h < 0.0f) h = 0.0f;
            return Vector2(w, h);
        };
        const float textPadX = 6.0f * s;

        auto styleInput = [&](UIInput& input)
        {
            input.paddingX = 6.0f * s;
            input.paddingY = 4.0f * s;
            input.background = field;
            input.hoverFill = hoverOf(field);
            input.border.color = t.ring;
            input.border.thickness = 1.0f;
            input.caret.color = ink;
            input.selection = t.selection;
            if (UIText* label = input.label())
            {
                styleText(*label, fontPath, fontPx, ink);
            }
            input.writeClipboard = mHost.writeClipboard;
            input.readClipboard = mHost.readClipboard;
        };
        auto centerText = [&](const std::shared_ptr<UIText>& text, float x, float width, float cellH)
        {
            centerFieldText(text, x, width, cellH, s);
        };

        UIWidget* overlayRaw = overlay;
        auto showPopup = [this, overlayRaw, s, fontPx, popup, fontPath](UIWidget& anchor, const std::vector<std::pair<const char*, std::function<void()>>>& entries)
        {
            if (overlayRaw == nullptr)
            {
                return;
            }
            auto menu = std::make_shared<UIMenu>();
            menu->direction = UIDirection::Vertical;
            menu->padding = 2.0f * s;
            menu->spacing = 1.0f * s;
            menu->color = popup;
            menu->overlay = true;
            menu->shape.pivot = Vector2(0.0f, 0.0f);
            menu->host = &anchor;
            for (const auto& entry : entries)
            {
                auto item = std::make_shared<UIMenuItem>();
                item->padding = 10.0f * s;
                item->setText(entry.first);
                styleMenuItem(*item, fontPath, menuFontPx(s));
                auto action = entry.second;
                UIMenu* menuRaw = menu.get();
                item->onClick = [menuRaw, action]()
                {
                    menuRaw->visible = false;
                    action();
                };
                menu->addItem(item);
            }
            overlayRaw->attachChild(menu);
            Rect bounds = anchor.screenBounds();
            Vector2 local = overlayRaw->screenToLocal(Vector2(bounds.origin.x, bounds.origin.y + bounds.size.y));
            menu->shape.setBox(local, Vector2(180.0f * s, 36.0f * s * static_cast<float>(entries.size()) + 12.0f * s));
        };

        UITableRow* header = table->row(0);
        const bool allSelected = mSelect == SelectKind::All;
        Vector2 cornerSize = innerBox(0, 0, headW, rowH);
        auto corner = std::make_shared<UIWidget>();
        corner->shape.pivot = Vector2(0.0f, 0.0f);
        corner->color = allSelected ? selected : headerBg;
        corner->focusable = true;
        setBox(corner, Rect(0.0f, 0.0f, cornerSize.x, cornerSize.y));
        corner->onClick = [this]() { this->selectAll(); };
        const bool emptyTable = rows == 0 || cols == 0;
        if (emptyTable)
        {
            const float moreW = 28.0f * s;
            const float moreH = 24.0f * s;
            const float moreGap = 4.0f * s;
            auto more = makeMoreButton(fontPath, fontPx, moreW, moreH, ink);
            setBox(more, Rect(cornerSize.x - moreW - moreGap, (cornerSize.y - moreH) * 0.5f, moreW, moreH));
            UIButton* moreRaw = more.get();
            more->onClick = [this, moreRaw, showPopup]()
            {
                showPopup(*moreRaw, {
                    {"Insert Row", [this]()
                    {
                        mDoc->instanceListInsert(0);
                        mSelect = SelectKind::Row;
                        mSelectRow = 0;
                        this->refreshUi();
                    }},
                });
            };
            corner->attachChild(more);
        }
        if (allSelected && mFocus == nullptr)
        {
            mFocus = corner.get();
        }
        paintCell(header->cell(0), 0, 0, allSelected ? selected : headerBg);
        header->setContent(0, corner);

        for (int col = 0; col < cols; ++col)
        {
            NestedField info = mDoc->instanceListColumn(static_cast<u32_t>(col));
            bool columnSelected = mSelect == SelectKind::Column && mSelectCol == col;
            Color headFill = (columnSelected || allSelected) ? selected : headerBg;
            Vector2 hitSize = innerBox(0, col + 1, colWs[static_cast<size_t>(col)], rowH);
            auto hit = std::make_shared<UIWidget>();
            hit->shape.pivot = Vector2(0.0f, 0.0f);
            hit->color = headFill;
            setBox(hit, Rect(0.0f, 0.0f, hitSize.x, hitSize.y));
            hit->onClick = [this, col]()
            {
                mClickHeader = true;
                mClickRowHead = false;
                mClickCol = col;
                mClickRow = -1;
                mSelect = SelectKind::Column;
                mSelectCol = col;
                this->refreshUi();
            };
            const float moreW = 28.0f * s;
            const float moreH = 24.0f * s;
            const float moreGap = 4.0f * s;
            float nameW = hitSize.x - textPadX * 2.0f;
            if (columnSelected)
            {
                nameW = hitSize.x - moreW - moreGap * 2.0f - textPadX;
                if (nameW < 0.0f)
                {
                    nameW = 0.0f;
                }
            }
            auto nameText = makeText(info.name, fontPath, fontPx, ink, nameW, 22.0f * s);
            centerText(nameText, textPadX, nameW, hitSize.y);
            hit->attachChild(nameText);
            if (columnSelected && mFocus == nullptr)
            {
                mFocus = hit.get();
            }
            paintCell(header->cell(col + 1), 0, col + 1, headFill);
            header->setContent(col + 1, hit);
        }

        for (int row = 0; row < rows; ++row)
        {
            bool rowSelected = mSelect == SelectKind::Row && mSelectRow == row;
            Color rowFill = (rowSelected || allSelected) ? selected : headerBg;
            Vector2 rowHitSize = innerBox(row + 1, 0, headW, rowH);
            auto rowHit = std::make_shared<UIWidget>();
            rowHit->shape.pivot = Vector2(0.0f, 0.0f);
            rowHit->color = rowFill;
            setBox(rowHit, Rect(0.0f, 0.0f, rowHitSize.x, rowHitSize.y));
            rowHit->onClick = [this, row]()
            {
                auto now = std::chrono::steady_clock::now();
                bool repeat = mClickRowHead && mClickRow == row && now - mClickTime < std::chrono::milliseconds(450);
                mClickTime = now;
                mClickRowHead = true;
                mClickHeader = false;
                mClickRow = row;
                mClickCol = -1;
                mSelect = SelectKind::Row;
                mSelectRow = row;
                mPickRow = -1;
                mPickCol = -1;

                this->refreshUi();
            };
            const float moreW = 28.0f * s;
            const float moreH = 24.0f * s;
            String rowLabel = String::format("%d", row + 1);
            float numMaxW = rowHitSize.x - textPadX;
            if (numMaxW < 1.0f)
            {
                numMaxW = 1.0f;
            }
            auto number = makeText(rowLabel, fontPath, fontPx, ink, numMaxW, 22.0f * s);
            number->shape.pivot = Vector2(0.0f, 0.0f);
            number->layout();
            float numW = number->shape.size.x;
            float numH = number->shape.size.y;
            if (numW < 1.0f)
            {
                numW = 1.0f;
            }
            if (numH < 1.0f)
            {
                numH = 22.0f * s;
            }
            setBox(number, Rect(rowHitSize.x - numW - textPadX, (rowHitSize.y - numH) * 0.5f, numW, numH));
            rowHit->attachChild(number);
            if (rowSelected)
            {
                auto more = makeMoreButton(fontPath, fontPx, moreW, moreH, ink);
                setBox(more, Rect(2.0f * s, (rowHitSize.y - moreH) * 0.5f, moreW, moreH));
                UIButton* moreRaw = more.get();
                more->onClick = [this, row, moreRaw, showPopup]()
                {
                    showPopup(*moreRaw, {
                        {"Delete Row", [this, row]() {
mDoc->instanceListRemove(static_cast<u32_t>(row));
                            mSelect = SelectKind::Cell;
                            mSelectRow = clampi(row, 0, static_cast<int>(mDoc->instanceListCount()) - 1);
                            mSelectCol = 0;
                            this->refreshUi();
                        }},
                        {"Insert Row Above", [this, row]() {
mDoc->instanceListInsert(static_cast<u32_t>(row));
                            mSelect = SelectKind::Row;
                            mSelectRow = row;
                            this->refreshUi();
                        }},
                        {"Insert Row Below", [this, row]() {
mDoc->instanceListInsert(static_cast<u32_t>(row + 1));
                            mSelect = SelectKind::Row;
                            mSelectRow = row + 1;
                            this->refreshUi();
                        }},
                    });
                };
                rowHit->attachChild(more);
            }
            paintCell(table->row(row + 1)->cell(0), row + 1, 0, rowFill);
            table->row(row + 1)->setContent(0, rowHit);

            for (int col = 0; col < cols; ++col)
            {
                NestedField info = mDoc->instanceListColumn(static_cast<u32_t>(col));
                bool cellSelected = mSelect == SelectKind::Cell && mSelectRow == row && mSelectCol == col;
                bool columnSelected = mSelect == SelectKind::Column && mSelectCol == col;
                bool editing = mEdit == EditKind::Cell && mEditRow == row && mEditCol == col;
                String shown = mDoc->instanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
                Color fill = paper;
                Color textColor = ink;
                if (info.kind == FieldKind::Url && !shown.isEmpty())
                {
                    textColor = url;
                }
                bool highlighted = cellSelected || rowSelected || columnSelected || allSelected;
                Vector2 hitSize = innerBox(row + 1, col + 1, colWs[static_cast<size_t>(col)], rowH);
                auto hit = std::make_shared<UIWidget>();
                hit->shape.pivot = Vector2(0.0f, 0.0f);
                hit->color = highlighted ? selectedCell : fill;
                setBox(hit, Rect(0.0f, 0.0f, hitSize.x, hitSize.y));
                hit->focusable = true;
                hit->onClick = [this, row, col, info]()
                {
                    if (mHost.ctrlDown && mHost.ctrlDown() && info.kind == FieldKind::Url && mHost.openExternal)
                    {
                        String link = mDoc->instanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
                        if (!link.isEmpty())
                        {
                            mHost.openExternal(link);
                            return;
                        }
                    }
                    this->selectCell(row, col);
                };
                hit->onKeyPress = [this, row, col, kind = info.kind](const UIKeyEvent& event)
                {
                    const UIKeyCode key = event.key;
                    const bool ctrl = event.ctrl;
                    if (key == UIKeyCode::Escape)
                    {
                        this->handleEscape();
                        return;
                    }
if (ctrl && key == UIKeyCode::C)
                    {
                        String text = mDoc->instanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
                        if (mHost.writeClipboard && !text.isEmpty())
                        {
                            mHost.writeClipboard(text);
                        }
                        return;
                    }
                    if (ctrl && key == UIKeyCode::V)
                    {
                        if (mHost.readClipboard)
                        {
                            mDoc->setInstanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), mHost.readClipboard());
                            this->refreshUi();
                        }
                        return;
                    }
                    if (key == UIKeyCode::Left) { mSelectCol = clampi(col - 1, 0, static_cast<int>(mDoc->instanceListColumnCount()) - 1); mSelectRow = row; mSelect = SelectKind::Cell; this->refreshUi(); return; }
                    if (key == UIKeyCode::Right || key == UIKeyCode::Tab) { mSelectCol = clampi(col + 1, 0, static_cast<int>(mDoc->instanceListColumnCount()) - 1); mSelectRow = row; mSelect = SelectKind::Cell; this->refreshUi(); return; }
                    if (key == UIKeyCode::Up) { mSelectRow = clampi(row - 1, 0, static_cast<int>(mDoc->instanceListCount()) - 1); mSelectCol = col; mSelect = SelectKind::Cell; this->refreshUi(); return; }
                    if (key == UIKeyCode::Down) { mSelectRow = clampi(row + 1, 0, static_cast<int>(mDoc->instanceListCount()) - 1); mSelectCol = col; mSelect = SelectKind::Cell; this->refreshUi(); return; }
                    if (kind == FieldKind::Boolean && (key == UIKeyCode::Enter || key == UIKeyCode::F2 || key == UIKeyCode::Space))
                    {
                        this->toggleBoolCell(row, col);
                        return;
                    }
                    if (key == UIKeyCode::Enter || key == UIKeyCode::F2)
                    {
this->beginEdit(row, col, mDoc->instanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col)));
                        return;
                    }
                    if (key == UIKeyCode::Delete || key == UIKeyCode::Backspace)
                    {
                        mDoc->setInstanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), String::empty);
                        this->refreshUi();
                    }
                };
                hit->onKeyChar = [this, row, col, kind = info.kind, readOnly = info.nestedStruct || info.nestedList](uint32_t character)
                {
                    if (readOnly || character < 32 || character == 127 || pickerKind(kind))
                    {
                        return;
                    }
                    this->beginEdit(row, col, UIFont::encodeUtf8(character));
                };
                if (info.kind == FieldKind::Boolean)
                {
                    auto toggle = makeBoolToggle(s, hitSize, boolFromText(shown));
                    toggle->onValueChanged = [this, row, col](bool value)
                    {
                        this->setBoolCell(row, col, value);
                    };
                    hit->attachChild(toggle);
                }
                else if (optionKind(info.kind))
                {
                    std::vector<String> labels;
                    if (info.kind == FieldKind::MultiOption)
                    {
                        labels = mDoc->instanceListCellLabels(static_cast<u32_t>(row), static_cast<u32_t>(col));
                    }
                    else if (!shown.isEmpty())
                    {
                        labels.push_back(shown);
                    }
                    attachOptionTags(*hit, hitSize, s, info.kind, info.options, labels, fontPath, smallPx,
                        [this, row, col, kind = info.kind](const String& label)
                        {
                            if (kind == FieldKind::SingleOption)
                            {
                                mDoc->setInstanceListCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), String::empty);
                            }
                            else
                            {
                                std::vector<String> current = mDoc->instanceListCellLabels(static_cast<u32_t>(row), static_cast<u32_t>(col));
                                std::vector<String> next;
                                for (const String& item : current)
                                {
                                    if (item != label)
                                    {
                                        next.push_back(item);
                                    }
                                }
                                mDoc->setInstanceListCellLabels(static_cast<u32_t>(row), static_cast<u32_t>(col), next);
                            }
                            this->refreshUi();
                        },
                        [this, row, col]() { this->selectCell(row, col); });
                }
                else if (info.kind == FieldKind::Color)
                {
                    attachColorValue(*hit, hitSize, s, shown, fontPath, fontPx);
                }
                else if (info.nestedStruct || info.nestedList)
                {
                    attachValueText(*hit, hitSize, s, shown, fontPath, fontPx, textColor);
                }
                else
                {
                    attachValueText(*hit, hitSize, s, shown, fontPath, fontPx, textColor);
                }
                if (editing)
                {
                    mEditAnchor = hit.get();
                    mEditFill = fill;
                }
                if (cellSelected && mPickRow == row && mPickCol == col && optionKind(info.kind))
                {
                    mPickAnchor = hit.get();
                }
                if (mDateRow == row && mDateCol == col)
                {
                    mDateAnchor = hit.get();
                }
                if (mColorRow == row && mColorCol == col)
                {
                    mColorAnchor = hit.get();
                }
                if (mFocus == nullptr && cellSelected && (!editing || pickerKind(info.kind)))
                {
                    mFocus = hit.get();
                }
                paintCell(table->row(row + 1)->cell(col + 1), row + 1, col + 1, fill);
                table->row(row + 1)->setContent(col + 1, hit);
            }
        }

        auto host = std::make_shared<UIWidget>();
        host->shape.pivot = Vector2(0.0f, 0.0f);
        host->pickable = false;
        setBox(host, Rect(0.0f, 0.0f, tableW, tableH));
        host->attachChild(table);
        mTable = table.get();
        mGridHost = host.get();
        mView = view.get();
        view->addChild(host);
        this->attachChild(view);
        view->setScroll(mKeepX, mKeepY);
        int revealRow = -1;
        int revealCol = -1;
        if (mEdit == EditKind::Cell)
        {
            revealRow = mEditRow;
            revealCol = mEditCol;
        }
        else if (mDateRow >= 0 && mDateCol >= 0)
        {
            revealRow = mDateRow;
            revealCol = mDateCol;
        }
        else if (mColorRow >= 0 && mColorCol >= 0)
        {
            revealRow = mColorRow;
            revealCol = mColorCol;
        }
        else if (mSelect == SelectKind::Cell)
        {
            revealRow = mSelectRow;
            revealCol = mSelectCol;
        }
        revealCell(*view, revealRow, revealCol, headW, colWs, rowH);

        if (overlay != nullptr && mEdit != EditKind::None && mEditAnchor != nullptr)
        {
            auto input = std::make_shared<EscapeInput>();
            input->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(input, Rect(0.0f, 0.0f, 40.0f, rowH));
            styleCellInput(*input, mEditFill, s, fontPath, fontPx, mHost.writeClipboard, mHost.readClipboard);
            input->setText(mEditSeed);
            UIInput* inputRaw = input.get();
            input->onSubmit = [this](const String& text) { this->finishEdit(text, 1, 0); };
            input->onEscape = [this]() { this->handleEscape(); };
            bindCommitOnFocusLoss(*input, mHost.isRebuildingUi, [this, inputRaw]()
            {
                if (mEdit != EditKind::None)
                {
                    this->finishEdit(inputRaw->text, 0, 0);
                }
            });
            input->onKeyPress = [this, inputRaw](const UIKeyEvent& event)
            {
if (event.key == UIKeyCode::Tab && mEdit == EditKind::Cell)
                {
                    this->finishEdit(inputRaw->text, 0, 1);
                }
            };
            overlay->attachChild(input);
            mEditInput = input.get();
            mFocus = input.get();
        }

        if (overlay != nullptr && mPickAnchor != nullptr && mPickRow >= 0 && mPickCol >= 0)
        {
            NestedField info = mDoc->instanceListColumn(static_cast<u32_t>(mPickCol));
            std::vector<String> selectedLabels = mDoc->instanceListCellLabels(static_cast<u32_t>(mPickRow), static_cast<u32_t>(mPickCol));
            int pickRow = mPickRow;
            int pickCol = mPickCol;
            FieldKind kind = info.kind;
            auto menu = makeOptionMenu(
                info.options,
                selectedLabels,
                s,
                fontPath,
                fontPx,
                colWs[static_cast<size_t>(mPickCol)],
                mPickAnchor,
                [this, pickRow, pickCol, kind](const String& label)
                {
                    if (kind == FieldKind::MultiOption)
                    {
                        std::vector<String> current = mDoc->instanceListCellLabels(static_cast<u32_t>(pickRow), static_cast<u32_t>(pickCol));
                        if (selectedContains(current, label))
                        {
                            std::vector<String> next;
                            for (const String& item : current)
                            {
                                if (item != label)
                                {
                                    next.push_back(item);
                                }
                            }
                            mDoc->setInstanceListCellLabels(static_cast<u32_t>(pickRow), static_cast<u32_t>(pickCol), next);
                        }
                        else
                        {
                            current.push_back(label);
                            mDoc->setInstanceListCellLabels(static_cast<u32_t>(pickRow), static_cast<u32_t>(pickCol), current);
                        }
                    }
                    else
                    {
                        mDoc->setInstanceListCellText(static_cast<u32_t>(pickRow), static_cast<u32_t>(pickCol), label);
                        mPickRow = -1;
                        mPickCol = -1;
                    }
                    this->refreshUi();
                },
                [this]()
                {
                    mPickRow = -1;
                    mPickCol = -1;
                });
            overlay->attachChild(menu);
            mPickMenu = menu.get();
        }

        if (overlay != nullptr && mDateRow >= 0 && mDateCol >= 0)
        {
            FieldKind dateKind = mDoc->instanceListColumn(static_cast<u32_t>(mDateCol)).kind;
            TemporalParts parts { mDateYear, mDateMonth, mDateDay, mDateHour, mDateMinute, mDateSecond };
            int dateRow = mDateRow;
            int dateCol = mDateCol;
            DatePanelWidgets widgets = makeDatePanel(
                dateKind,
                parts,
                s,
                fontPath,
                fontPx,
                smallPx,
                [this]() { mDateRow = -1; mDateCol = -1; this->refreshUi(); },
                [this, dateRow, dateCol]()
                {
                    mDoc->setInstanceListCellText(static_cast<u32_t>(dateRow), static_cast<u32_t>(dateCol), String::empty);
                    mDateRow = -1;
                    mDateCol = -1;
                    this->refreshUi();
                },
                [this, dateRow, dateCol](const String& stored)
                {
                    mDoc->setInstanceListCellText(static_cast<u32_t>(dateRow), static_cast<u32_t>(dateCol), stored);
                    mDateRow = -1;
                    mDateCol = -1;
                    this->refreshUi();
                },
                [this]() { this->handleEscape(); },
                [this](const TemporalParts& next)
                {
                    mDateYear = next.year;
                    mDateMonth = next.month;
                    mDateDay = next.day;
                    mDateHour = next.hour;
                    mDateMinute = next.minute;
                    mDateSecond = next.second;
                });
            overlay->attachChild(widgets.panel);
            mDatePanel = widgets.panel.get();
            mFocus = widgets.focus;
        }

        if (overlay != nullptr && mColorRow >= 0 && mColorCol >= 0)
        {
            int colorRow = mColorRow;
            int colorCol = mColorCol;
            Color current(1.0f, 1.0f, 1.0f, 1.0f);
            parseColor(mDoc->instanceListCellText(static_cast<u32_t>(mColorRow), static_cast<u32_t>(mColorCol)), current);
            ColorPanelWidgets widgets = makeColorPanel(
                current,
                s,
                fontPath,
                fontPx,
                smallPx,
                mHost.writeClipboard,
                mHost.readClipboard,
                [this, colorRow, colorCol](const Color& value)
                {
                    mDoc->setInstanceListCellText(static_cast<u32_t>(colorRow), static_cast<u32_t>(colorCol), formatColor(value));
                    mColorRow = -1;
                    mColorCol = -1;
                    this->refreshUi();
                },
                [this, colorRow, colorCol]()
                {
                    mDoc->setInstanceListCellText(static_cast<u32_t>(colorRow), static_cast<u32_t>(colorCol), String::empty);
                    mColorRow = -1;
                    mColorCol = -1;
                    this->refreshUi();
                },
                [this]() { mColorRow = -1; mColorCol = -1; this->refreshUi(); },
                [this]() { this->handleEscape(); });
            overlay->attachChild(widgets.panel);
            mColorPanel = widgets.panel.get();
            mColorPicker = widgets.picker;
            mFocus = widgets.focus;
        }
    }

}
