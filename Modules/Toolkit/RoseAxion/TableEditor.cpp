#include "./TableEditor.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "Base/Time.h"
#include "UI/Main.h"
#include "UI/UIFont.h"

#include <cmath>
#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    namespace {

        bool optionKind(FieldKind kind)
        {
            return kind == FieldKind::SingleOption || kind == FieldKind::MultiOption;
        }

        bool temporalKind(FieldKind kind)
        {
            return kind == FieldKind::Date || kind == FieldKind::Time || kind == FieldKind::DateTime;
        }

        bool pathKind(FieldKind kind)
        {
            return kind == FieldKind::FilePath || kind == FieldKind::FolderPath;
        }

        bool pickerKind(FieldKind kind)
        {
            return kind == FieldKind::Boolean || kind == FieldKind::Color || temporalKind(kind) || pathKind(kind);
        }

        int clampi(int value, int lo, int hi)
        {
            if (value < lo) return lo;
            if (value > hi) return hi;
            return value;
        }

        bool boolFromText(const String& text)
        {
            bool flag = false;
            return parseBool(text, flag) && flag;
        }

        bool colorFromHex(const String& text, Color& out)
        {
            return parseColor(text, out);
        }

        Color invertRgb(const Color& color)
        {
            return Color(1.0f - color.r, 1.0f - color.g, 1.0f - color.b, 1.0f);
        }

        Color optionChipColor(const String& hex)
        {
            Color chip = theme().chip;
            colorFromHex(hex, chip);
            return chip;
        }

        struct EscapeInput : UIInput
        {
            std::function<void()> onEscape;
            bool handleKeyPress(const UIKeyEvent& event) override
            {
                if (event.key == UIKeyCode::Escape)
                {
                    if (onEscape)
                    {
                        onEscape();
                    }
                    return true;
                }
                return UIInput::handleKeyPress(event);
            }
        };

        struct EscapeColorPicker : UIColorPicker
        {
            std::function<void()> onEscape;
            bool handleKeyPress(const UIKeyEvent& event) override
            {
                if (UIColorPicker::handleKeyPress(event))
                {
                    return true;
                }
                if (event.key == UIKeyCode::Escape)
                {
                    if (onEscape)
                    {
                        onEscape();
                    }
                    return true;
                }
                return false;
            }
        };

        struct SettingsDrawer : UIWidget
        {
            UIWidget* footer = nullptr;
            float pad = 16.0f;
            void layout() override
            {
                if (!visible)
                {
                    return;
                }
                for (auto& child : children())
                {
                    if (!child || child.get() == footer)
                    {
                        continue;
                    }
                    child->layout();
                }
                if (footer == nullptr)
                {
                    return;
                }
                footer->layout();
                float y = shape.size.y - pad - footer->shape.size.y;
                if (y < pad)
                {
                    y = pad;
                }
                this->placeChild(*footer, Vector2(floorf(pad + 0.5f), floorf(y + 0.5f)));
            }
        };

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

        void placeOnOverlay(UIWidget& overlay, UIWidget& widget, const Rect& screen, bool below)
        {
            Vector2 topLeft = overlay.screenToLocal(Vector2(screen.origin.x, below ? screen.origin.y + screen.size.y : screen.origin.y));
            if (!below)
            {
                widget.shape.setBox(topLeft, screen.size);
                return;
            }
            float w = widget.shape.size.x;
            if (w < screen.size.x)
            {
                w = screen.size.x;
            }
            widget.shape.setBox(topLeft, Vector2(w, widget.shape.size.y));
        }

        void bindEscape(UIWidget& widget, const std::function<void()>& onEscape)
        {
            if (dynamic_cast<UINumberPicker*>(&widget) != nullptr)
            {
                widget.onKeyPress = [onEscape](const UIKeyEvent& event)
                {
                    if (event.key == UIKeyCode::Escape)
                    {
                        onEscape();
                    }
                };
            }
            for (const auto& child : widget.children())
            {
                if (child)
                {
                    bindEscape(*child, onEscape);
                }
            }
        }

        template<typename T>
        void styleTemporalPicker(T& picker, float s, const String& font, float fontSize, const Color& field, const Color& ink, const Color& mute, const Color& accent)
        {
            picker.shape.pivot = Vector2(0.0f, 0.0f);
            picker.uiScale = s;
            picker.direction = UIDirection::Vertical;
            picker.setFont(font, fontSize);
            picker.background = field;
            picker.field = field;
            picker.ink = ink;
            picker.muted = mute;
            picker.accent = accent;
            picker.button = theme().quiet;
            picker.buttonHover = hoverOf(theme().quiet);
            picker.buttonPress = pressOf(theme().quiet);
            picker.line = theme().line;
        }

    }

    void RoseTableEditor::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseTableEditor::setOverlay(UIWidget* overlay)
    {
        if (mOverlay == overlay)
        {
            return;
        }
        if (mOverlay != nullptr)
        {
            if (mEditInput != nullptr)
            {
                mOverlay->detachChild(mEditInput);
            }
            if (mPickMenu != nullptr)
            {
                mOverlay->detachChild(mPickMenu);
            }
            if (mDatePanel != nullptr)
            {
                mOverlay->detachChild(mDatePanel);
            }
            if (mColorPanel != nullptr)
            {
                mOverlay->detachChild(mColorPanel);
            }
            if (mSettingsPanel != nullptr)
            {
                mOverlay->detachChild(mSettingsPanel);
            }
        }
        mEditInput = nullptr;
        mPickMenu = nullptr;
        mDatePanel = nullptr;
        mColorPanel = nullptr;
        mColorPicker = nullptr;
        mSettingsPanel = nullptr;
        mOverlay = overlay;
    }

    const String& RoseTableEditor::font() const
    {
        return mHost.font;
    }

    float RoseTableEditor::scale() const
    {
        return mHost.scale < 0.5f ? 1.0f : mHost.scale;
    }

    void RoseTableEditor::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseTableEditor::reset()
    {
        mSelect = SelectKind::Cell;
        mSelectRow = 0;
        mSelectCol = 0;
        mEdit = EditKind::None;
        mSettingsCol = -1;
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
        mResizeCol = -1;
        this->setOverlay(nullptr);
        mView = nullptr;
        mTable = nullptr;
        mGridHost = nullptr;
        mEditAnchor = nullptr;
        mPickAnchor = nullptr;
        mDateAnchor = nullptr;
        mColorAnchor = nullptr;
        mFocus = nullptr;
        mResizeHandles.clear();
        this->detachChildren();
    }

    void RoseTableEditor::closeSettings()
    {
        mSettingsCol = -1;
        mColorRow = -1;
        mColorCol = -1;
    }

    float RoseTableEditor::scrollX() const
    {
        return mView != nullptr ? mView->scrollX() : mKeepX;
    }

    float RoseTableEditor::scrollY() const
    {
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseTableEditor::setScroll(float x, float y)
    {
        mKeepX = x;
        mKeepY = y;
        if (mView != nullptr)
        {
            mView->setScroll(x, y);
        }
    }

    void RoseTableEditor::layout()
    {
        if (!visible)
        {
            return;
        }
        if (mView != nullptr)
        {
            setBox(*mView, Rect(0.0f, 0.0f, shape.size.x, shape.size.y));
            mView->layout();
        }
        this->placeOverlayEditors();
        UIWidget::layout();
    }

    void RoseTableEditor::placeOverlayEditors()
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
        if (mSettingsPanel != nullptr && mOverlay != nullptr)
        {
            float w = mSettingsPanel->shape.size.x;
            float h = mOverlay->shape.size.y;
            float x = mOverlay->shape.size.x - w;
            if (x < 0.0f)
            {
                x = 0.0f;
            }
            setBox(*mSettingsPanel, Rect(x, 0.0f, w, h));
        }
    }

    void RoseTableEditor::clampState()
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        int rows = static_cast<int>(mDoc->rowCount());
        int cols = static_cast<int>(mDoc->columnCount());
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
        if (mSettingsCol >= cols) mSettingsCol = -1;
        if (mDateRow >= rows || mDateCol >= cols) { mDateRow = -1; mDateCol = -1; }
        else if (mDateCol >= 0 && cols > 0 && !temporalKind(mDoc->column(static_cast<u32_t>(mDateCol)).kind)) { mDateRow = -1; mDateCol = -1; }
        if (mPickRow >= rows || mPickCol >= cols) { mPickRow = -1; mPickCol = -1; }
        if (mColorRow >= rows || mColorCol >= cols) { mColorRow = -1; mColorCol = -1; }
        else if (mColorCol >= 0 && cols > 0 && mDoc->column(static_cast<u32_t>(mColorCol)).kind != FieldKind::Color) { mColorRow = -1; mColorCol = -1; }
        if (mEdit == EditKind::Cell && (rows <= 0 || cols <= 0 || mEditRow >= rows || mEditCol >= cols)) mEdit = EditKind::None;
        if (mEdit == EditKind::Cell && cols > 0 && pickerKind(mDoc->column(static_cast<u32_t>(mEditCol)).kind)) mEdit = EditKind::None;
        if (mEdit == EditKind::Header && (cols <= 0 || mEditCol >= cols)) mEdit = EditKind::None;
    }

    void RoseTableEditor::finishEdit(const String& text, int rowDelta, int colDelta)
    {
        if (mEdit == EditKind::None || mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        if (mEdit == EditKind::Header)
        {
            mDoc->setColumnName(static_cast<u32_t>(mEditCol), text);
            mSelect = SelectKind::Cell;
            mSelectRow = 0;
            mSelectCol = mEditCol;
        }
        else
        {
            mDoc->setCellText(static_cast<u32_t>(mEditRow), static_cast<u32_t>(mEditCol), text);
            int rowHi = static_cast<int>(mDoc->rowCount()) - 1;
            int colHi = static_cast<int>(mDoc->columnCount()) - 1;
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
        this->notifyRebuild();
    }

    void RoseTableEditor::beginEdit(int row, int col, const String& seed)
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        if (mDoc->isNestedColumn(static_cast<u32_t>(col)))
        {
            return;
        }
        FieldKind kind = mDoc->column(static_cast<u32_t>(col)).kind;
        mSelect = SelectKind::Cell;
        mSelectRow = row;
        mSelectCol = col;
        mEdit = EditKind::None;
        if (temporalKind(kind))
        {
            TimePoint now;
            mDateYear = now.year();
            mDateMonth = now.month();
            mDateDay = now.date();
            mDateHour = now.hour();
            mDateMinute = now.minute();
            mDateSecond = now.second();
            String shown = mDoc->cellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
            if (kind == FieldKind::Date)
            {
                parseDate(shown, mDateYear, mDateMonth, mDateDay);
            }
            else if (kind == FieldKind::Time)
            {
                parseTime(shown, mDateHour, mDateMinute, mDateSecond);
            }
            else
            {
                parseDateTime(shown, mDateYear, mDateMonth, mDateDay, mDateHour, mDateMinute, mDateSecond);
            }
            mDateRow = row;
            mDateCol = col;
            mColorRow = -1;
            mColorCol = -1;
            mSettingsCol = -1;
            mPickRow = -1;
            mPickCol = -1;
            this->notifyRebuild();
            return;
        }
        if (kind == FieldKind::Color)
        {
            mColorRow = row;
            mColorCol = col;
            mDateRow = -1;
            mDateCol = -1;
            mSettingsCol = -1;
            mPickRow = -1;
            mPickCol = -1;
            this->notifyRebuild();
            return;
        }
        if (kind == FieldKind::Boolean)
        {
            return;
        }
        if (pathKind(kind))
        {
            mSettingsCol = -1;
            mPickRow = -1;
            mPickCol = -1;
            mDateRow = -1;
            mDateCol = -1;
            mColorRow = -1;
            mColorCol = -1;
            String current = mDoc->cellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
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
                mDoc->setCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), picked);
            }
            this->notifyRebuild();
            return;
        }
        if (optionKind(kind))
        {
            mPickRow = row;
            mPickCol = col;
            mSettingsCol = -1;
            mDateRow = -1;
            mDateCol = -1;
            mColorRow = -1;
            mColorCol = -1;
            this->notifyRebuild();
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
        this->notifyRebuild();
    }

    void RoseTableEditor::selectCell(int row, int col)
    {
        auto now = std::chrono::steady_clock::now();
        bool repeat = !mClickHeader && mClickRow == row && mClickCol == col && now - mClickTime < std::chrono::milliseconds(450);
        mClickTime = now;
        mClickRow = row;
        mClickCol = col;
        mClickHeader = false;
        mSelect = SelectKind::Cell;
        mSelectRow = row;
        mSelectCol = col;
        if (repeat)
        {
            this->beginEdit(row, col, mDoc->cellText(static_cast<u32_t>(row), static_cast<u32_t>(col)));
            return;
        }
        mPickRow = -1;
        mPickCol = -1;
        this->notifyRebuild();
    }

    void RoseTableEditor::setBoolCell(int row, int col, bool value)
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        mDoc->setCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), value ? "true" : "false");
        mSelect = SelectKind::Cell;
        mSelectRow = row;
        mSelectCol = col;
        mEdit = EditKind::None;
        mPickRow = -1;
        mPickCol = -1;
        this->notifyRebuild();
    }

    void RoseTableEditor::toggleBoolCell(int row, int col)
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        this->setBoolCell(row, col, !boolFromText(mDoc->cellText(static_cast<u32_t>(row), static_cast<u32_t>(col))));
    }

    void RoseTableEditor::selectAll()
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        mSelect = SelectKind::All;
        mPickRow = -1;
        mPickCol = -1;
        mClickHeader = false;
        mClickRow = -1;
        mClickCol = -1;
        this->notifyRebuild();
    }

    bool RoseTableEditor::handleEscape()
    {
        if (mResizeCol >= 0)
        {
            this->cancelResize();
            return true;
        }
        if (mDateRow >= 0 || mDateCol >= 0)
        {
            mDateRow = -1;
            mDateCol = -1;
            this->notifyRebuild();
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
            this->notifyRebuild();
            return true;
        }
        if (mEdit != EditKind::None)
        {
            mEdit = EditKind::None;
            this->notifyRebuild();
            return true;
        }
        if (mPickRow >= 0 || mPickCol >= 0)
        {
            mPickRow = -1;
            mPickCol = -1;
            this->notifyRebuild();
            return true;
        }
        return false;
    }

    bool RoseTableEditor::colorDropping() const
    {
        return mColorPicker != nullptr && mColorPicker->eyeDropping();
    }

    void RoseTableEditor::sampleColorDropper(float x, float y, bool commit)
    {
        if (mColorPicker != nullptr)
        {
            mColorPicker->sampleEyeDropper(x, y, commit);
        }
    }

    void RoseTableEditor::cancelColorDropper()
    {
        if (mColorPicker != nullptr)
        {
            mColorPicker->cancelEyeDropper();
        }
    }

    bool RoseTableEditor::resizeCursorAt(float x, float y) const
    {
        if (mResizeCol >= 0)
        {
            return true;
        }
        int tableCol = -1;
        return this->hitResizeCol(x, y, tableCol);
    }

    bool RoseTableEditor::hitResizeCol(float screenX, float screenY, int& tableCol) const
    {
        tableCol = -1;
        if (mTable == nullptr)
        {
            return false;
        }
        Vector2 local = mTable->screenToLocal(Vector2(screenX, screenY));
        if (local.y < 0.0f || local.y > mTable->shape.size.y)
        {
            return false;
        }
        const float grip = 6.0f * this->scale();
        float right = 0.0f;
        int count = mTable->columnCount();
        for (int i = 0; i < count; ++i)
        {
            right += mTable->columnWidth(i);
            if (local.x >= right - grip * 0.5f && local.x <= right + grip * 0.5f)
            {
                tableCol = i;
                return true;
            }
        }
        return false;
    }

    void RoseTableEditor::placeResizeHandles()
    {
        if (mTable == nullptr || mGridHost == nullptr)
        {
            return;
        }
        const float grip = 6.0f * this->scale();
        float right = 0.0f;
        float tableH = mTable->shape.size.y;
        int count = mTable->columnCount();
        for (int i = 0; i < count && i < static_cast<int>(mResizeHandles.size()); ++i)
        {
            right += mTable->columnWidth(i);
            setBox(*mResizeHandles[static_cast<size_t>(i)], Rect(right - grip * 0.5f, 0.0f, grip, tableH));
        }
    }

    void RoseTableEditor::applyTableWidth(int tableCol, float pixelWidth)
    {
        if (mTable == nullptr || mGridHost == nullptr || mView == nullptr)
        {
            return;
        }
        float minW = 48.0f * this->scale();
        if (pixelWidth < minW)
        {
            pixelWidth = minW;
        }
        mTable->setColumnWidth(tableCol, pixelWidth);
        float tableW = 0.0f;
        int count = mTable->columnCount();
        for (int i = 0; i < count; ++i)
        {
            tableW += mTable->columnWidth(i);
        }
        float tableH = mTable->shape.size.y;
        setBox(*mTable, Rect(0.0f, 0.0f, tableW, tableH));
        setBox(*mGridHost, Rect(0.0f, 0.0f, tableW, tableH));
        mTable->layout();
        this->placeResizeHandles();
        mView->layout();
        this->placeOverlayEditors();
    }

    void RoseTableEditor::commitResize()
    {
        if (mResizeCol < 0 || mTable == nullptr || mDoc == nullptr || !mDoc->isOpen())
        {
            mResizeCol = -1;
            return;
        }
        float logical = mTable->columnWidth(mResizeCol) / this->scale();
        if (mResizeCol == 0)
        {
            mDoc->setHeadWidth(logical);
        }
        else
        {
            mDoc->setColumnWidth(static_cast<u32_t>(mResizeCol - 1), logical);
        }
        mResizeCol = -1;
        this->notifyRebuild();
    }

    void RoseTableEditor::cancelResize()
    {
        if (mResizeCol >= 0)
        {
            this->applyTableWidth(mResizeCol, mResizeOriginW);
        }
        mResizeCol = -1;
    }

    void RoseTableEditor::rebuild()
    {
        this->detachChildren();
        mView = nullptr;
        mTable = nullptr;
        mGridHost = nullptr;
        mEditAnchor = nullptr;
        mPickAnchor = nullptr;
        mDateAnchor = nullptr;
        mColorAnchor = nullptr;
        mPickMenu = nullptr;
        mEditInput = nullptr;
        mDatePanel = nullptr;
        mColorPanel = nullptr;
        mColorPicker = nullptr;
        mSettingsPanel = nullptr;
        mFocus = nullptr;
        mResizeHandles.clear();
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveTable())
        {
            return;
        }
        this->clampState();

        const float s = this->scale();
        const float fontPx = 15.0f * s;
        const float smallPx = 13.0f * s;
        const float rowH = 36.0f * s;
        const float drawerW = 360.0f * s;
        const RoseTheme& t = theme();
        const Color canvas = t.canvas;
        const Color paper = t.paper;
        const Color headerBg = t.header;
        const Color selected = t.selected;
        const Color selectedCell = t.selectedCell;
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color accent = t.accent;
        const Color field = t.field;
        const Color popup = t.popup;
        const Color url = t.url;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const Color quiet = t.quiet;
        const String& fontPath = this->font();
        this->color = canvas;
        const float height = shape.size.y;
        UIWidget* overlay = mOverlay;

        int rows = static_cast<int>(mDoc->rowCount());
        int cols = static_cast<int>(mDoc->columnCount());
        const float headW = mDoc->headWidth() * s;
        std::vector<float> colWs(static_cast<size_t>(cols), 0.0f);
        for (int col = 0; col < cols; ++col)
        {
            colWs[static_cast<size_t>(col)] = mDoc->columnWidth(static_cast<u32_t>(col)) * s;
        }

        auto view = std::make_shared<UIView>();
        view->shape.pivot = Vector2(0.0f, 0.0f);
        view->color = canvas;
        setBox(view, Rect(0.0f, 0.0f, shape.size.x, height));

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
        auto styleCellInput = [&](UIInput& input, const Color& fill)
        {
            input.paddingX = textPadX;
            input.paddingY = 0.0f;
            input.background = fill;
            input.hoverFill = fill;
            input.border.color = clear;
            input.border.thickness = 0.0f;
            input.caret.color = ink;
            input.selection = t.selection;
            if (UIText* label = input.label())
            {
                styleText(*label, fontPath, fontPx, ink);
            }
            input.writeClipboard = mHost.writeClipboard;
            input.readClipboard = mHost.readClipboard;
        };
        auto styleToggle = [&](UIToggle& toggle)
        {
            toggle.off = quiet;
            toggle.on = accent;
            toggle.offHover = hoverOf(quiet);
            toggle.onHover = hoverOf(accent);
            toggle.offPressed = pressOf(quiet);
            toggle.onPressed = pressOf(accent);
            toggle.thumb = ink;
        };
        auto centerText = [&](const std::shared_ptr<UIText>& text, float x, float width, float cellH)
        {
            text->shape.pivot = Vector2(0.0f, 0.0f);
            text->layout();
            float h = text->shape.size.y;
            if (h < 1.0f)
            {
                h = 22.0f * s;
            }
            float w = text->shape.size.x;
            if (w < 1.0f || w > width)
            {
                w = width;
            }
            setBox(text, Rect(x, (cellH - h) * 0.5f, w, h));
        };
        auto makeOptionTag = [&](int row, int col, FieldKind kind, const String& label, const std::vector<FieldOption>& options)
        {
            Color chip = optionChipColor(String::empty);
            for (const FieldOption& option : options)
            {
                if (option.label != label)
                {
                    continue;
                }
                chip = optionChipColor(option.color);
                break;
            }
            auto tag = std::make_shared<UITag>();
            tag->shape.pivot = Vector2(0.0f, 0.0f);
            tag->paddingX = 6.0f * s;
            tag->paddingY = 3.0f * s;
            tag->closeGap = 4.0f * s;
            tag->background = chip;
            tag->caption().text = label;
            styleText(tag->caption(), fontPath, smallPx, invertRgb(chip));
            tag->onClose = [this, row, col, label, kind]()
            {
                if (kind == FieldKind::SingleOption)
                {
                    mDoc->setCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), String::empty);
                }
                else
                {
                    std::vector<String> current = mDoc->cellLabels(static_cast<u32_t>(row), static_cast<u32_t>(col));
                    std::vector<String> next;
                    for (const String& item : current)
                    {
                        if (item != label)
                        {
                            next.push_back(item);
                        }
                    }
                    mDoc->setCellLabels(static_cast<u32_t>(row), static_cast<u32_t>(col), next);
                }
                this->notifyRebuild();
            };
            tag->onClick = [this, row, col]()
            {
                this->selectCell(row, col);
            };
            return tag;
        };
        auto attachOptionTags = [&](UIWidget& hit, const Vector2& hitSize, int row, int col, const TableColumn& info, const std::vector<String>& labels)
        {
            auto tags = std::make_shared<UIList>();
            tags->direction = UIDirection::Horizontal;
            tags->padding = textPadX;
            tags->spacing = 4.0f * s;
            tags->centerCross = true;
            tags->color = clear;
            tags->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(tags, Rect(0.0f, 0.0f, hitSize.x, hitSize.y));
            for (const String& label : labels)
            {
                tags->attachChild(makeOptionTag(row, col, info.kind, label, info.options));
            }
            hit.attachChild(tags);
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
            menu->padding = 4.0f * s;
            menu->spacing = 2.0f * s;
            menu->color = popup;
            menu->overlay = true;
            menu->shape.pivot = Vector2(0.0f, 0.0f);
            menu->host = &anchor;
            for (const auto& entry : entries)
            {
                auto item = std::make_shared<UIMenuItem>();
                item->paddingX = 12.0f * s;
                item->paddingY = 8.0f * s;
                item->setText(entry.first);
                styleMenuItem(*item, fontPath, fontPx);
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
        const float moreW = 28.0f * s;
        const float moreH = 24.0f * s;
        const float moreGap = 4.0f * s;
        float nameW = cornerSize.x - textPadX * 2.0f;
        if (emptyTable)
        {
            nameW = cornerSize.x - moreW - moreGap * 2.0f - textPadX;
            if (nameW < 0.0f)
            {
                nameW = 0.0f;
            }
        }
        auto tableName = makeText(mDoc->tableName(mDoc->activeTableIndex()), fontPath, fontPx, ink, nameW, 22.0f * s);
        centerText(tableName, textPadX, nameW, cornerSize.y);
        corner->attachChild(tableName);
        if (emptyTable)
        {
            auto more = makeMoreButton(fontPath, fontPx, moreW, moreH, ink);
            setBox(more, Rect(cornerSize.x - moreW - moreGap, (cornerSize.y - moreH) * 0.5f, moreW, moreH));
            UIButton* moreRaw = more.get();
            more->onClick = [this, moreRaw, showPopup, rows, cols]()
            {
                std::vector<std::pair<const char*, std::function<void()>>> entries;
                if (cols == 0)
                {
                    entries.push_back({"插入一列", [this]()
                    {
                        mDoc->insertColumn(0);
                        mSelect = SelectKind::Column;
                        mSelectCol = 0;
                        this->notifyRebuild();
                    }});
                }
                if (rows == 0)
                {
                    entries.push_back({"插入一行", [this]()
                    {
                        if (mDoc->columnCount() == 0)
                        {
                            mDoc->insertColumn(0);
                            mSelect = SelectKind::Column;
                            mSelectCol = 0;
                        }
                        else
                        {
                            mDoc->insertRow(0);
                            mSelect = SelectKind::Row;
                            mSelectRow = 0;
                        }
                        this->notifyRebuild();
                    }});
                }
                showPopup(*moreRaw, entries);
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
            TableColumn info = mDoc->column(static_cast<u32_t>(col));
            bool columnSelected = mSelect == SelectKind::Column && mSelectCol == col;
            Color headFill = (columnSelected || allSelected) ? selected : headerBg;
            Vector2 hitSize = innerBox(0, col + 1, colWs[static_cast<size_t>(col)], rowH);
            auto hit = std::make_shared<UIWidget>();
            hit->shape.pivot = Vector2(0.0f, 0.0f);
            hit->color = headFill;
            setBox(hit, Rect(0.0f, 0.0f, hitSize.x, hitSize.y));
            hit->onClick = [this, col]()
            {
                auto now = std::chrono::steady_clock::now();
                bool repeat = mClickHeader && mClickCol == col && now - mClickTime < std::chrono::milliseconds(450);
                mClickTime = now;
                mClickHeader = true;
                mClickCol = col;
                mClickRow = -1;
                mSelect = SelectKind::Column;
                mSelectCol = col;
                if (repeat)
                {
                    mEdit = EditKind::Header;
                    mEditCol = col;
                    mEditSeed = mDoc->column(static_cast<u32_t>(col)).name;
                    this->notifyRebuild();
                    return;
                }
                this->notifyRebuild();
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
            if (columnSelected)
            {
                auto more = makeMoreButton(fontPath, fontPx, moreW, moreH, ink);
                setBox(more, Rect(hitSize.x - moreW - moreGap, (hitSize.y - moreH) * 0.5f, moreW, moreH));
                UIButton* moreRaw = more.get();
                more->onClick = [this, col, moreRaw, showPopup]()
                {
                    showPopup(*moreRaw, {
                        {"字段设置", [this, col]() { mSettingsCol = col; mDateRow = -1; mDateCol = -1; mColorRow = -1; mColorCol = -1; this->notifyRebuild(); }},
                        {"在左边插入一列", [this, col]() { mDoc->insertColumn(static_cast<u32_t>(col)); mSelect = SelectKind::Column; mSelectCol = col; this->notifyRebuild(); }},
                        {"在右边插入一列", [this, col]() { mDoc->insertColumn(static_cast<u32_t>(col + 1)); mSelect = SelectKind::Column; mSelectCol = col + 1; this->notifyRebuild(); }},
                    });
                };
                hit->attachChild(more);
            }
            if (mEdit == EditKind::Header && mEditCol == col)
            {
                mEditAnchor = hit.get();
                mEditFill = headFill;
            }
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
                mSelect = SelectKind::Row;
                mSelectRow = row;
                mPickRow = -1;
                mPickCol = -1;
                this->notifyRebuild();
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
                        {"删除当前行", [this, row]() { mDoc->deleteRow(static_cast<u32_t>(row)); mSelect = SelectKind::Cell; mSelectRow = clampi(row, 0, static_cast<int>(mDoc->rowCount()) - 1); mSelectCol = 0; this->notifyRebuild(); }},
                        {"在上方插入一行", [this, row]() { mDoc->insertRow(static_cast<u32_t>(row)); mSelect = SelectKind::Row; mSelectRow = row; this->notifyRebuild(); }},
                        {"在下方插入一行", [this, row]() { mDoc->insertRow(static_cast<u32_t>(row + 1)); mSelect = SelectKind::Row; mSelectRow = row + 1; this->notifyRebuild(); }},
                    });
                };
                rowHit->attachChild(more);
            }
            paintCell(table->row(row + 1)->cell(0), row + 1, 0, rowFill);
            table->row(row + 1)->setContent(0, rowHit);

            for (int col = 0; col < cols; ++col)
            {
                TableColumn info = mDoc->column(static_cast<u32_t>(col));
                bool cellSelected = mSelect == SelectKind::Cell && mSelectRow == row && mSelectCol == col;
                bool columnSelected = mSelect == SelectKind::Column && mSelectCol == col;
                bool editing = mEdit == EditKind::Cell && mEditRow == row && mEditCol == col;
                String shown = mDoc->cellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
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
                        String link = mDoc->cellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
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
                    if (mHost.handleShortcut && mHost.handleShortcut(key, ctrl))
                    {
                        return;
                    }
                    if (ctrl && key == UIKeyCode::C)
                    {
                        String text = mDoc->cellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
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
                            mDoc->setCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), mHost.readClipboard());
                            this->notifyRebuild();
                        }
                        return;
                    }
                    if (key == UIKeyCode::Left) { mSelectCol = clampi(col - 1, 0, static_cast<int>(mDoc->columnCount()) - 1); mSelectRow = row; mSelect = SelectKind::Cell; this->notifyRebuild(); return; }
                    if (key == UIKeyCode::Right || key == UIKeyCode::Tab) { mSelectCol = clampi(col + 1, 0, static_cast<int>(mDoc->columnCount()) - 1); mSelectRow = row; mSelect = SelectKind::Cell; this->notifyRebuild(); return; }
                    if (key == UIKeyCode::Up) { mSelectRow = clampi(row - 1, 0, static_cast<int>(mDoc->rowCount()) - 1); mSelectCol = col; mSelect = SelectKind::Cell; this->notifyRebuild(); return; }
                    if (key == UIKeyCode::Down) { mSelectRow = clampi(row + 1, 0, static_cast<int>(mDoc->rowCount()) - 1); mSelectCol = col; mSelect = SelectKind::Cell; this->notifyRebuild(); return; }
                    if (kind == FieldKind::Boolean && (key == UIKeyCode::Enter || key == UIKeyCode::F2 || key == UIKeyCode::Space))
                    {
                        this->toggleBoolCell(row, col);
                        return;
                    }
                    if (key == UIKeyCode::Enter || key == UIKeyCode::F2)
                    {
                        this->beginEdit(row, col, mDoc->cellText(static_cast<u32_t>(row), static_cast<u32_t>(col)));
                        return;
                    }
                    if (key == UIKeyCode::Delete || key == UIKeyCode::Backspace)
                    {
                        mDoc->setCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), String::empty);
                        this->notifyRebuild();
                    }
                };
                hit->onKeyChar = [this, row, col, kind = info.kind](uint32_t character)
                {
                    if (character < 32 || character == 127 || pickerKind(kind))
                    {
                        return;
                    }
                    this->beginEdit(row, col, UIFont::encodeUtf8(character));
                };
                if (info.kind == FieldKind::Boolean)
                {
                    float toggleH = 22.0f * s;
                    float toggleW = 40.0f * s;
                    if (toggleH > hitSize.y - 4.0f * s)
                    {
                        toggleH = hitSize.y - 4.0f * s;
                    }
                    if (toggleH < 1.0f)
                    {
                        toggleH = 1.0f;
                    }
                    if (toggleW > hitSize.x - textPadX * 2.0f)
                    {
                        toggleW = hitSize.x - textPadX * 2.0f;
                    }
                    if (toggleW < toggleH)
                    {
                        toggleW = toggleH;
                    }
                    auto toggle = std::make_shared<UIToggle>();
                    toggle->shape.pivot = Vector2(0.0f, 0.0f);
                    setBox(toggle, Rect(textPadX, (hitSize.y - toggleH) * 0.5f, toggleW, toggleH));
                    styleToggle(*toggle);
                    toggle->value = boolFromText(shown);
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
                        labels = mDoc->cellLabels(static_cast<u32_t>(row), static_cast<u32_t>(col));
                    }
                    else if (!shown.isEmpty())
                    {
                        labels.push_back(shown);
                    }
                    attachOptionTags(*hit, hitSize, row, col, info, labels);
                }
                else if (info.kind == FieldKind::Color)
                {
                    Color chip;
                    if (parseColor(shown, chip))
                    {
                        float swatch = 14.0f * s;
                        if (swatch > hitSize.y - 4.0f * s)
                        {
                            swatch = hitSize.y - 4.0f * s;
                        }
                        if (swatch < 1.0f)
                        {
                            swatch = 1.0f;
                        }
                        float borderW = 1.0f * s;
                        if (borderW > swatch * 0.25f)
                        {
                            borderW = swatch * 0.25f;
                        }
                        auto border = std::make_shared<UIWidget>();
                        border->pickable = false;
                        border->shape.pivot = Vector2(0.0f, 0.0f);
                        border->color = t.voidBg;
                        setBox(border, Rect(textPadX, (hitSize.y - swatch) * 0.5f, swatch, swatch));
                        auto swatchFill = std::make_shared<UIWidget>();
                        swatchFill->pickable = false;
                        swatchFill->shape.pivot = Vector2(0.0f, 0.0f);
                        swatchFill->color = chip;
                        float inner = swatch - borderW * 2.0f;
                        if (inner < 1.0f)
                        {
                            inner = 1.0f;
                        }
                        setBox(swatchFill, Rect(borderW, borderW, inner, inner));
                        border->attachChild(swatchFill);
                        hit->attachChild(border);
                        String hex = formatColor(chip);
                        float labelX = textPadX + swatch + 6.0f * s;
                        float labelW = hitSize.x - labelX - textPadX;
                        if (labelW < 1.0f)
                        {
                            labelW = 1.0f;
                        }
                        auto label = makeText(hex, fontPath, fontPx, chip, labelW, 22.0f * s);
                        centerText(label, labelX, labelW, hitSize.y);
                        hit->attachChild(label);
                    }
                }
                else
                {
                    auto label = makeText(shown, fontPath, fontPx, textColor, hitSize.x - textPadX * 2.0f, 22.0f * s);
                    centerText(label, textPadX, hitSize.x - textPadX * 2.0f, hitSize.y);
                    hit->attachChild(label);
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
        mResizeHandles.clear();
        int tableCols = cols + 1;
        for (int tableCol = 0; tableCol < tableCols; ++tableCol)
        {
            auto handle = std::make_shared<UIWidget>();
            handle->overlay = true;
            handle->pickable = true;
            handle->color = clear;
            handle->shape.pivot = Vector2(0.0f, 0.0f);
            handle->onPointerPress = [this, tableCol](const UIPointerEvent& event)
            {
                if (event.pointerCode != UIPointerCode::Left || mTable == nullptr)
                {
                    return;
                }
                mResizeCol = tableCol;
                mResizeOriginX = event.position.x;
                mResizeOriginW = mTable->columnWidth(tableCol);
            };
            handle->onDrag = [this, tableCol](const Vector2&, const Vector2& delta)
            {
                if (mResizeCol != tableCol)
                {
                    return;
                }
                this->applyTableWidth(tableCol, mResizeOriginW + delta.x);
            };
            handle->onPointerRelease = [this](const UIPointerEvent&)
            {
                this->commitResize();
            };
            handle->onDrop = [this](const Vector2&, const Vector2&)
            {
                this->commitResize();
            };
            host->attachChild(handle);
            mResizeHandles.push_back(handle);
        }
        this->placeResizeHandles();
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
            styleCellInput(*input, mEditFill);
            input->setText(mEditSeed);
            UIInput* inputRaw = input.get();
            input->onSubmit = [this](const String& text) { this->finishEdit(text, mEdit == EditKind::Header ? 0 : 1, 0); };
            input->onEscape = [this]() { this->handleEscape(); };
            input->onFocusLoss = [this, inputRaw]()
            {
                if (mEdit != EditKind::None)
                {
                    this->finishEdit(inputRaw->text, 0, 0);
                }
            };
            input->onKeyPress = [this, inputRaw](const UIKeyEvent& event)
            {
                if (mHost.handleShortcut && mHost.handleShortcut(event.key, event.ctrl))
                {
                    return;
                }
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
            TableColumn info = mDoc->column(static_cast<u32_t>(mPickCol));
            auto menu = std::make_shared<UIMenu>();
            menu->direction = UIDirection::Vertical;
            menu->padding = 4.0f * s;
            menu->spacing = 2.0f * s;
            menu->color = popup;
            menu->overlay = true;
            menu->shape.pivot = Vector2(0.0f, 0.0f);
            menu->host = mPickAnchor;
            if (info.options.empty())
            {
                auto item = std::make_shared<UIMenuItem>();
                item->setText("无可选选项");
                styleMenuItem(*item, fontPath, fontPx);
                if (UIText* text = item->label()) styleText(*text, fontPath, fontPx, mute);
                menu->addItem(item);
            }
            std::vector<String> selectedLabels = mDoc->cellLabels(static_cast<u32_t>(mPickRow), static_cast<u32_t>(mPickCol));
            for (const FieldOption& option : info.options)
            {
                auto item = std::make_shared<UIMenuItem>();
                item->paddingX = 10.0f * s;
                item->paddingY = 6.0f * s;
                bool on = selectedContains(selectedLabels, option.label);
                item->setText(option.label);
                styleMenuItem(*item, fontPath, fontPx);
                item->selected = on;
                item->showChip = true;
                item->chip = optionChipColor(option.color);
                item->chipSize = 18.0f * s;
                item->chipGap = 8.0f * s;
                String label = option.label;
                FieldKind kind = info.kind;
                int pickRow = mPickRow;
                int pickCol = mPickCol;
                item->onClick = [this, pickRow, pickCol, label, kind]()
                {
                    if (kind == FieldKind::MultiOption)
                    {
                        std::vector<String> current = mDoc->cellLabels(static_cast<u32_t>(pickRow), static_cast<u32_t>(pickCol));
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
                            mDoc->setCellLabels(static_cast<u32_t>(pickRow), static_cast<u32_t>(pickCol), next);
                        }
                        else
                        {
                            current.push_back(label);
                            mDoc->setCellLabels(static_cast<u32_t>(pickRow), static_cast<u32_t>(pickCol), current);
                        }
                    }
                    else
                    {
                        mDoc->setCellText(static_cast<u32_t>(pickRow), static_cast<u32_t>(pickCol), label);
                        mPickRow = -1;
                        mPickCol = -1;
                    }
                    this->notifyRebuild();
                };
                menu->addItem(item);
            }
            float pickH = 32.0f * s * static_cast<float>(info.options.empty() ? 1 : info.options.size()) + 8.0f * s;
            menu->shape.setBox(Vector2(0.0f, 0.0f), Vector2(colWs[static_cast<size_t>(mPickCol)], pickH));
            UIMenu* menuRaw = menu.get();
            menu->onPointerPressAnywhere = [this, menuRaw](const UIPointerEvent&)
            {
                if (!menuRaw->visible)
                {
                    mPickRow = -1;
                    mPickCol = -1;
                }
            };
            overlay->attachChild(menu);
            mPickMenu = menu.get();
        }

        if (overlay != nullptr && mDateRow >= 0 && mDateCol >= 0)
        {
            FieldKind dateKind = mDoc->column(static_cast<u32_t>(mDateCol)).kind;
            auto panel = std::make_shared<UIList>();
            panel->direction = UIDirection::Vertical;
            panel->padding = 16.0f * s;
            panel->spacing = 10.0f * s;
            panel->pickable = true;
            panel->color = t.raised;
            panel->shape.pivot = Vector2(0.0f, 0.0f);
            Vector2 pickerSize = UIDateTimePicker::preferredSize(s, true);
            const char* titleText = "日期时间选择";
            if (dateKind == FieldKind::Date)
            {
                pickerSize = UIDatePicker::preferredSize(s);
                titleText = "日期选择";
            }
            else if (dateKind == FieldKind::Time)
            {
                pickerSize = UITimePicker::preferredSize(s, true);
                titleText = "时间选择";
            }
            float panelW = pickerSize.x + 32.0f * s;
            float panelH = pickerSize.y + 116.0f * s;
            setBox(panel, Rect(0.0f, 0.0f, panelW, panelH));
            float innerW = panelW - 32.0f * s;
            auto head = std::make_shared<UIWidget>();
            head->pickable = false;
            head->shape.pivot = Vector2(0.0f, 0.0f);
            head->color = clear;
            setBox(head, Rect(0.0f, 0.0f, innerW, 32.0f * s));
            auto title = makeText(titleText, fontPath, fontPx + 1.0f, ink, innerW - 76.0f * s, 28.0f * s);
            title->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(title, Rect(0.0f, 2.0f * s, innerW - 76.0f * s, 28.0f * s));
            head->attachChild(title);
            auto close = makeButton("关闭", fontPath, smallPx, 64.0f * s, 28.0f * s, quiet, ink);
            close->shape.setBox(Vector2(innerW - 64.0f * s, 2.0f * s), Vector2(64.0f * s, 28.0f * s));
            close->onClick = [this]() { mDateRow = -1; mDateCol = -1; this->notifyRebuild(); };
            head->attachChild(close);
            panel->attachChild(head);
            std::shared_ptr<UIWidget> pickerWidget;
            std::function<void()> capturePicker;
            auto escapePicker = [this]() { this->handleEscape(); };
            auto pickerKeyPress = [escapePicker](const UIKeyEvent& event)
            {
                if (event.key == UIKeyCode::Escape)
                {
                    escapePicker();
                }
            };
            if (dateKind == FieldKind::Date)
            {
                auto picker = std::make_shared<UIDatePicker>();
                styleTemporalPicker(*picker, s, fontPath, smallPx, field, ink, mute, accent);
                setBox(picker, Rect(0.0f, 0.0f, pickerSize.x, pickerSize.y));
                picker->setDate(mDateYear, mDateMonth, mDateDay);
                mDateYear = picker->year();
                mDateMonth = picker->month();
                mDateDay = picker->day();
                UIDatePicker* pickerRaw = picker.get();
                capturePicker = [this, pickerRaw]()
                {
                    mDateYear = pickerRaw->year();
                    mDateMonth = pickerRaw->month();
                    mDateDay = pickerRaw->day();
                };
                picker->onDateChanged = capturePicker;
                picker->onDateCommitted = capturePicker;
                picker->onKeyPress = pickerKeyPress;
                bindEscape(*picker, escapePicker);
                pickerWidget = picker;
            }
            else if (dateKind == FieldKind::Time)
            {
                auto picker = std::make_shared<UITimePicker>();
                styleTemporalPicker(*picker, s, fontPath, smallPx, field, ink, mute, accent);
                picker->showSeconds = true;
                setBox(picker, Rect(0.0f, 0.0f, pickerSize.x, pickerSize.y));
                picker->setTime(mDateHour, mDateMinute, mDateSecond);
                mDateHour = picker->hour();
                mDateMinute = picker->minute();
                mDateSecond = picker->second();
                UITimePicker* pickerRaw = picker.get();
                capturePicker = [this, pickerRaw]()
                {
                    mDateHour = pickerRaw->hour();
                    mDateMinute = pickerRaw->minute();
                    mDateSecond = pickerRaw->second();
                };
                picker->onTimeChanged = capturePicker;
                picker->onTimeCommitted = capturePicker;
                picker->onKeyPress = pickerKeyPress;
                bindEscape(*picker, escapePicker);
                pickerWidget = picker;
            }
            else
            {
                auto picker = std::make_shared<UIDateTimePicker>();
                styleTemporalPicker(*picker, s, fontPath, smallPx, field, ink, mute, accent);
                picker->showSeconds = true;
                setBox(picker, Rect(0.0f, 0.0f, pickerSize.x, pickerSize.y));
                picker->setDateTime(mDateYear, mDateMonth, mDateDay, mDateHour, mDateMinute, mDateSecond);
                mDateYear = picker->year();
                mDateMonth = picker->month();
                mDateDay = picker->day();
                mDateHour = picker->hour();
                mDateMinute = picker->minute();
                mDateSecond = picker->second();
                UIDateTimePicker* pickerRaw = picker.get();
                capturePicker = [this, pickerRaw]()
                {
                    mDateYear = pickerRaw->year();
                    mDateMonth = pickerRaw->month();
                    mDateDay = pickerRaw->day();
                    mDateHour = pickerRaw->hour();
                    mDateMinute = pickerRaw->minute();
                    mDateSecond = pickerRaw->second();
                };
                picker->onDateTimeChanged = capturePicker;
                picker->onDateTimeCommitted = capturePicker;
                picker->onKeyPress = pickerKeyPress;
                bindEscape(*picker, escapePicker);
                pickerWidget = picker;
            }
            panel->attachChild(pickerWidget);
            auto actions = std::make_shared<UIList>();
            actions->direction = UIDirection::Horizontal;
            actions->padding = 0.0f;
            actions->spacing = 8.0f * s;
            actions->color = clear;
            actions->shape.pivot = Vector2(0.0f, 0.0f);
            int dateRow = mDateRow;
            int dateCol = mDateCol;
            auto clearBtn = makeButton("清除", fontPath, fontPx, 80.0f * s, 32.0f * s, quiet, ink);
            clearBtn->onClick = [this, dateRow, dateCol]()
            {
                mDoc->setCellText(static_cast<u32_t>(dateRow), static_cast<u32_t>(dateCol), String::empty);
                mDateRow = -1;
                mDateCol = -1;
                this->notifyRebuild();
            };
            auto ok = makeButton("确定", fontPath, fontPx, 80.0f * s, 32.0f * s, accent, t.inkOnAccent);
            ok->onClick = [this, dateRow, dateCol, dateKind, capturePicker]()
            {
                if (capturePicker)
                {
                    capturePicker();
                }
                String stored = formatDateTime(mDateYear, mDateMonth, mDateDay, mDateHour, mDateMinute, mDateSecond);
                if (dateKind == FieldKind::Date)
                {
                    stored = formatDate(mDateYear, mDateMonth, mDateDay);
                }
                else if (dateKind == FieldKind::Time)
                {
                    stored = formatTime(mDateHour, mDateMinute, mDateSecond);
                }
                mDoc->setCellText(static_cast<u32_t>(dateRow), static_cast<u32_t>(dateCol), stored);
                mDateRow = -1;
                mDateCol = -1;
                this->notifyRebuild();
            };
            close->onKeyPress = pickerKeyPress;
            clearBtn->onKeyPress = pickerKeyPress;
            ok->onKeyPress = pickerKeyPress;
            actions->attachChild(clearBtn);
            actions->attachChild(ok);
            panel->attachChild(actions);
            overlay->attachChild(panel);
            mDatePanel = panel.get();
            mFocus = pickerWidget.get();
        }

        if (overlay != nullptr && mColorRow >= 0 && mColorCol >= 0)
        {
            auto panel = std::make_shared<UIList>();
            panel->direction = UIDirection::Vertical;
            panel->padding = 16.0f * s;
            panel->spacing = 10.0f * s;
            panel->pickable = true;
            panel->color = t.raised;
            panel->shape.pivot = Vector2(0.0f, 0.0f);
            Vector2 pickerSize = UIColorPicker::preferredSize(s);
            float panelW = pickerSize.x + 32.0f * s;
            float panelH = pickerSize.y + 74.0f * s;
            setBox(panel, Rect(0.0f, 0.0f, panelW, panelH));
            float innerW = panelW - 32.0f * s;
            float btnW = 64.0f * s;
            float btnH = 28.0f * s;
            float btnGap = 8.0f * s;
            float headActions = btnW * 2.0f + btnGap + 12.0f * s;
            auto head = std::make_shared<UIWidget>();
            head->pickable = false;
            head->shape.pivot = Vector2(0.0f, 0.0f);
            head->color = clear;
            setBox(head, Rect(0.0f, 0.0f, innerW, 32.0f * s));
            auto title = makeText("颜色选择", fontPath, fontPx + 1.0f, ink, innerW - headActions, 28.0f * s);
            title->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(title, Rect(0.0f, 2.0f * s, innerW - headActions, 28.0f * s));
            head->attachChild(title);
            int colorRow = mColorRow;
            int colorCol = mColorCol;
            auto clearBtn = makeButton("清除", fontPath, smallPx, btnW, btnH, quiet, ink);
            clearBtn->shape.setBox(Vector2(innerW - btnW * 2.0f - btnGap, 2.0f * s), Vector2(btnW, btnH));
            clearBtn->onClick = [this, colorRow, colorCol]()
            {
                mDoc->setCellText(static_cast<u32_t>(colorRow), static_cast<u32_t>(colorCol), String::empty);
                mColorRow = -1;
                mColorCol = -1;
                this->notifyRebuild();
            };
            head->attachChild(clearBtn);
            auto close = makeButton("关闭", fontPath, smallPx, btnW, btnH, quiet, ink);
            close->shape.setBox(Vector2(innerW - btnW, 2.0f * s), Vector2(btnW, btnH));
            close->onClick = [this]() { mColorRow = -1; mColorCol = -1; this->notifyRebuild(); };
            head->attachChild(close);
            panel->attachChild(head);
            auto picker = std::make_shared<EscapeColorPicker>();
            picker->shape.pivot = Vector2(0.0f, 0.0f);
            picker->uiScale = s;
            picker->useAlpha = true;
            picker->setFont(fontPath, smallPx);
            picker->background = field;
            picker->field = field;
            picker->ink = ink;
            picker->muted = mute;
            picker->accent = accent;
            picker->button = quiet;
            picker->buttonHover = hoverOf(quiet);
            picker->buttonPress = pressOf(quiet);
            picker->line = t.line;
            picker->writeClipboard = mHost.writeClipboard;
            picker->readClipboard = mHost.readClipboard;
            Color current(1.0f, 1.0f, 1.0f, 1.0f);
            parseColor(mDoc->cellText(static_cast<u32_t>(mColorRow), static_cast<u32_t>(mColorCol)), current);
            picker->setColor(current, true);
            setBox(picker, Rect(0.0f, 0.0f, pickerSize.x, pickerSize.y));
            picker->onColorCommitted = [this, colorRow, colorCol](const Color& value)
            {
                mDoc->setCellText(static_cast<u32_t>(colorRow), static_cast<u32_t>(colorCol), formatColor(value));
                mColorRow = -1;
                mColorCol = -1;
                this->notifyRebuild();
            };
            picker->onCancelled = [this]()
            {
                mColorRow = -1;
                mColorCol = -1;
                this->notifyRebuild();
            };
            auto escapePicker = [this]() { this->handleEscape(); };
            picker->onEscape = escapePicker;
            picker->onKeyPress = [escapePicker](const UIKeyEvent& event)
            {
                if (event.key == UIKeyCode::Escape)
                {
                    escapePicker();
                }
            };
            bindEscape(*picker, escapePicker);
            panel->attachChild(picker);
            close->onKeyPress = picker->onKeyPress;
            clearBtn->onKeyPress = picker->onKeyPress;
            overlay->attachChild(panel);
            mColorPanel = panel.get();
            mColorPicker = picker.get();
            mFocus = picker.get();
        }

        if (overlay != nullptr && mSettingsCol >= 0)
        {
            TableColumn info = mDoc->column(static_cast<u32_t>(mSettingsCol));
            auto panel = std::make_shared<SettingsDrawer>();
            panel->pickable = true;
            panel->color = t.raised;
            panel->shape.pivot = Vector2(0.0f, 0.0f);
            panel->pad = 16.0f * s;
            setBox(panel, Rect(0.0f, 0.0f, drawerW, shape.size.y > 1.0f ? shape.size.y : 400.0f * s));
            float innerW = drawerW - 32.0f * s;
            auto body = std::make_shared<UIList>();
            body->direction = UIDirection::Vertical;
            body->padding = 16.0f * s;
            body->spacing = 10.0f * s;
            body->color = clear;
            body->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(body, Rect(0.0f, 0.0f, drawerW, 0.0f));
            auto head = std::make_shared<UIWidget>();
            head->pickable = false;
            head->shape.pivot = Vector2(0.0f, 0.0f);
            head->color = clear;
            setBox(head, Rect(0.0f, 0.0f, innerW, 32.0f * s));
            auto title = makeText("字段设置", fontPath, fontPx + 1.0f, ink, innerW - 76.0f * s, 28.0f * s);
            title->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(title, Rect(0.0f, 2.0f * s, innerW - 76.0f * s, 28.0f * s));
            head->attachChild(title);
            auto close = makeButton("关闭", fontPath, smallPx, 64.0f * s, 28.0f * s, quiet, ink);
            close->shape.setBox(Vector2(innerW - 64.0f * s, 2.0f * s), Vector2(64.0f * s, 28.0f * s));
            close->onClick = [this]() { mSettingsCol = -1; this->notifyRebuild(); };
            head->attachChild(close);
            body->attachChild(head);
            body->attachChild(makeText("字段名称", fontPath, smallPx, mute, 200.0f * s, 18.0f * s));
            auto name = std::make_shared<UIInput>();
            name->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(name, Rect(0.0f, 0.0f, drawerW - 40.0f * s, 32.0f * s));
            styleInput(*name);
            name->setText(info.name);
            int settingsCol = mSettingsCol;
            UIInput* nameRaw = name.get();
            name->onFocusLoss = [this, nameRaw, settingsCol]()
            {
                mDoc->setColumnName(static_cast<u32_t>(settingsCol), nameRaw->text);
                this->notifyRebuild();
            };
            name->onSubmit = [this, settingsCol](const String& text)
            {
                mDoc->setColumnName(static_cast<u32_t>(settingsCol), text);
                this->notifyRebuild();
            };
            body->attachChild(name);
            body->attachChild(makeText("字段类型", fontPath, smallPx, mute, 200.0f * s, 18.0f * s));
            auto drop = std::make_shared<UIDropdown>();
            drop->overlay = true;
            drop->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(drop, Rect(0.0f, 0.0f, drawerW - 40.0f * s, 32.0f * s));
            styleDropdown(*drop, fontPath, fontPx);
            std::vector<String> typeNames = mDoc->columnTypeNames();
            for (int i = 0; i < static_cast<int>(typeNames.size()); ++i)
            {
                drop->addItem(i, schemaLabel(typeNames[static_cast<size_t>(i)]));
            }
            int current = 0;
            for (int i = 0; i < static_cast<int>(typeNames.size()); ++i)
            {
                if (typeNames[static_cast<size_t>(i)] == info.schemaName) current = i;
            }
            drop->setValue(current);
            drop->onValueChanged = [this, settingsCol, typeNames](int index)
            {
                if (index < 0 || index >= static_cast<int>(typeNames.size())) return;
                const String& typeName = typeNames[static_cast<size_t>(index)];
                if (typeName == mDoc->column(static_cast<u32_t>(settingsCol)).schemaName) return;
                if (mDoc->columnChangeClears(static_cast<u32_t>(settingsCol), typeName))
                {
                    if (mHost.confirmChangeType)
                    {
                        mHost.confirmChangeType(settingsCol, typeName);
                    }
                }
                else
                {
                    mDoc->setColumnType(static_cast<u32_t>(settingsCol), typeName);
                }
                this->notifyRebuild();
            };
            body->attachChild(drop);
            panel->attachChild(body);

            auto removeField = makeDangerButton("删除字段", fontPath, fontPx, innerW, 32.0f * s);
            setBox(removeField, Rect(0.0f, 0.0f, innerW, 32.0f * s));
            removeField->onClick = [this, settingsCol]()
            {
                if (mHost.confirmDeleteColumn)
                {
                    mHost.confirmDeleteColumn(settingsCol);
                }
            };
            panel->attachChild(removeField);
            panel->footer = removeField.get();
            overlay->attachChild(panel);
            mSettingsPanel = panel.get();
        }
    }

}
