#ifndef _EOKAS_ROSE_AXION_INSTANCE_EDITOR_LIST_GRID_H_
#define _EOKAS_ROSE_AXION_INSTANCE_EDITOR_LIST_GRID_H_

#include "./Document.h"
#include "./Host.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <chrono>
#include <memory>
#include <vector>

namespace eokas {
    class UIColorPicker;
    class UIView;
    class UITable;
}

namespace eokas::RoseAxion {

    class RoseInstanceEditor_ListGrid : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
            std::function<void(const String&)> openExternal;
            std::function<String(const String& suggested)> pickFilePath;
            std::function<String(const String& suggested)> pickFolderPath;
            std::function<bool()> ctrlDown;
            UIWidget* overlay = nullptr;
        };

        void bind(TableDocument& doc, const Host& host);
        void setOverlay(UIWidget* overlay);
        void reset();
        void rebuild();
        bool handleEscape();
        bool colorDropping() const;
        void sampleColorDropper(float x, float y, bool commit);
        void cancelColorDropper();
        float scrollX() const;
        float scrollY() const;
        void setScroll(float x, float y);
        UIWidget* focusWidget() const { return mFocus; }
        bool hasPopupEdit() const;
        void placeOverlayEditors();
        void layout() override;

    private:
        enum class SelectKind { None, Cell, Row, Column, All };
        enum class EditKind { None, Cell };

        void clampState();
        void finishEdit(const String& text, int rowDelta, int colDelta);
        void beginEdit(int row, int col, const String& seed);
        void selectCell(int row, int col);
        void setBoolCell(int row, int col, bool value);
        void toggleBoolCell(int row, int col);
        void selectAll();
        void notifyRebuild();
        void refreshUi();
        const String& font() const;
        float scale() const;
        bool isReadOnlyColumn(u32_t col) const;
        void clearOverlayEditors();

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIView* mView = nullptr;
        UITable* mTable = nullptr;
        UIWidget* mGridHost = nullptr;
        UIWidget* mOverlay = nullptr;
        UIWidget* mEditAnchor = nullptr;
        UIWidget* mPickAnchor = nullptr;
        UIWidget* mDateAnchor = nullptr;
        UIWidget* mColorAnchor = nullptr;
        UIWidget* mPickMenu = nullptr;
        UIWidget* mEditInput = nullptr;
        float mBarH = 0.0f;
        UIWidget* mDatePanel = nullptr;
        UIWidget* mColorPanel = nullptr;
        UIColorPicker* mColorPicker = nullptr;
        UIWidget* mFocus = nullptr;
        Color mEditFill { 0.145f, 0.157f, 0.196f, 1.0f };

        SelectKind mSelect = SelectKind::None;
        EditKind mEdit = EditKind::None;
        int mSelectRow = 0;
        int mSelectCol = 0;
        int mEditRow = 0;
        int mEditCol = 0;
        String mEditSeed;
        int mDateRow = -1;
        int mDateCol = -1;
        int mPickRow = -1;
        int mPickCol = -1;
        int mColorRow = -1;
        int mColorCol = -1;
        int mDateYear = 0;
        int mDateMonth = 0;
        int mDateDay = 0;
        int mDateHour = 0;
        int mDateMinute = 0;
        int mDateSecond = 0;
        bool mClickHeader = false;
        bool mClickRowHead = false;
        int mClickRow = -1;
        int mClickCol = -1;
        std::chrono::steady_clock::time_point mClickTime {};
        float mKeepX = 0.0f;
        float mKeepY = 0.0f;
    };

}

#endif