#ifndef _EOKAS_ROSE_AXION_APP_H_
#define _EOKAS_ROSE_AXION_APP_H_

#include "./Document.h"

#include "UI/UIEvent.h"

#include <chrono>
#include <functional>

namespace eokas {
    class UIWindow;
}

namespace eokas::RoseAxion {

    struct AxionHost
    {
        std::function<String()> pickOpenPath;
        std::function<String(const String& suggested)> pickSavePath;
        std::function<void(const String&)> writeClipboard;
        std::function<String()> readClipboard;
        std::function<void(const String&)> openExternal;
        std::function<bool()> ctrlDown;
        std::function<void(const String&)> setTitle;
    };

    class AxionApp
    {
    public:
        void attach(UIWindow& window, const AxionHost& host, const String& fontPath, float scale);
        void resize(float width, float height);
        void tick();

    private:
        void requestRebuild();
        void build();
        void notifyTitle();
        bool handleShortcut(UIKeyCode key, bool ctrl);
        void finishEdit(const String& text, int rowDelta, int colDelta);
        void beginEdit(int row, int col, const String& seed);
        void selectCell(int row, int col);
        void selectAll();
        void newDocument();
        void openDocument();
        void saveDocument();
        String fileTitle() const;
        bool handleEscape();

        UIWindow* mWindow = nullptr;
        AxionHost mHost;
        String mFont;
        float mWidth = 1280.0f;
        float mHeight = 800.0f;
        float mScale = 1.0f;
        bool mRebuild = true;
        TableDocument mDoc;

        enum class SelectKind { None, Cell, Row, Column, All };
        enum class EditKind { None, Cell, Header };
        enum class ConfirmKind { None, ChangeType, DeleteColumn };

        SelectKind mSelect = SelectKind::None;
        EditKind mEdit = EditKind::None;
        ConfirmKind mConfirm = ConfirmKind::None;
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
        FieldKind mPendingKind = FieldKind::Text;
        String mDateDraft;
        String mTimeDraft;
        std::chrono::steady_clock::time_point mClickTime {};
        int mClickRow = -1;
        int mClickCol = -1;
        bool mClickHeader = false;
        bool mResetScroll = false;
    };

}

#endif//_EOKAS_ROSE_AXION_APP_H_
