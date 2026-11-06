#ifndef _EOKAS_ROSE_AXION_EDITOR_H_
#define _EOKAS_ROSE_AXION_EDITOR_H_

#include "./Document.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace eokas {
    class UITable;
    class UIView;
}

namespace eokas::RoseAxion {

    class RoseEditor : public UIWidget
    {
    public:
        struct Host
        {
            std::function<void()> requestRebuild;
            std::function<void(const String&)> writeClipboard;
            std::function<String()> readClipboard;
            std::function<void(const String&)> openExternal;
            std::function<bool()> ctrlDown;
            std::function<bool(UIKeyCode, bool)> handleShortcut;
            std::function<void(int, FieldKind)> confirmChangeType;
            std::function<void(int)> confirmDeleteColumn;
            String font;
            float scale = 1.0f;
        };

        void bind(TableDocument& doc, const Host& host);
        void reset();
        void closeSettings();
        void rebuild();
        bool resizeCursorAt(float x, float y) const;
        bool handleEscape();
        float scrollX() const;
        float scrollY() const;
        void setScroll(float x, float y);
        UIWidget* focusWidget() const { return mFocus; }

        void layout() override;

    private:
        enum class SelectKind { None, Cell, Row, Column, All };
        enum class EditKind { None, Cell, Header };

        void clampState();
        void finishEdit(const String& text, int rowDelta, int colDelta);
        void beginEdit(int row, int col, const String& seed);
        void selectCell(int row, int col);
        void setBoolCell(int row, int col, bool value);
        void toggleBoolCell(int row, int col);
        void selectAll();
        void commitResize();
        void cancelResize();
        void applyTableWidth(int tableCol, float pixelWidth);
        void placeResizeHandles();
        bool hitResizeCol(float screenX, float screenY, int& tableCol) const;
        void placeOverlayEditors();
        void notifyRebuild();
        const String& font() const;
        float scale() const;

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIView* mView = nullptr;
        UITable* mTable = nullptr;
        UIWidget* mGridHost = nullptr;
        UIWidget* mOverlay = nullptr;
        UIWidget* mEditAnchor = nullptr;
        UIWidget* mPickAnchor = nullptr;
        UIWidget* mDateAnchor = nullptr;
        UIWidget* mPickMenu = nullptr;
        UIWidget* mEditInput = nullptr;
        UIWidget* mDatePanel = nullptr;
        UIWidget* mSettingsPanel = nullptr;
        UIWidget* mFocus = nullptr;
        Color mEditFill { 0.145f, 0.157f, 0.196f, 1.0f };

        SelectKind mSelect = SelectKind::None;
        EditKind mEdit = EditKind::None;
        int mSelectRow = 0;
        int mSelectCol = 0;
        int mEditRow = 0;
        int mEditCol = 0;
        String mEditSeed;
        int mSettingsCol = -1;
        int mDateRow = -1;
        int mDateCol = -1;
        int mPickRow = -1;
        int mPickCol = -1;
        int mColorIndex = -1;
        String mDateDraft;
        String mTimeDraft;
        std::chrono::steady_clock::time_point mClickTime {};
        int mClickRow = -1;
        int mClickCol = -1;
        bool mClickHeader = false;
        float mKeepX = 0.0f;
        float mKeepY = 0.0f;
        int mResizeCol = -1;
        float mResizeOriginX = 0.0f;
        float mResizeOriginW = 0.0f;
        std::vector<std::shared_ptr<UIWidget>> mResizeHandles;
    };

}

#endif
