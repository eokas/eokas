#ifndef _EOKAS_ROSE_AXION_APP_H_
#define _EOKAS_ROSE_AXION_APP_H_

#include "./Document.h"
#include "./Editor.h"

#include "UI/UIEvent.h"

#include <functional>
#include <memory>

namespace eokas {
    class UIWindow;
    class UIWidget;
}

namespace eokas::RoseAxion {

    struct AxionHost
    {
        std::function<String()> pickOpenPath;
        std::function<String(const String& suggested)> pickSavePath;
        std::function<void(const String&)> writeClipboard;
        std::function<String()> readClipboard;
        std::function<void(const String&)> openExternal;
        std::function<String(const String& suggested)> pickFilePath;
        std::function<String(const String& suggested)> pickFolderPath;
        std::function<bool()> ctrlDown;
        std::function<void(const String&)> setTitle;
        std::function<void()> requestQuit;
    };

    class AxionApp
    {
    public:
        void attach(UIWindow& window, const AxionHost& host, const String& fontPath, float scale);
        void resize(float width, float height);
        void tick();
        bool resizeCursorAt(float x, float y) const;
        bool colorDropping() const;
        void sampleColorDropper(float x, float y, bool commit);
        void cancelColorDropper();
        bool requestClose();

    private:
        enum class ConfirmKind { None, ChangeType, DeleteColumn, DeleteTable, DeleteSchema, Unsaved };
        enum class PendingAction { None, New, Open, Close };

        void requestRebuild();
        void build();
        void notifyTitle();
        bool handleShortcut(UIKeyCode key, bool ctrl);
        void resetView();
        bool confirmUnsaved(PendingAction action);
        void runPending();
        void applyNewDocument();
        void applyOpenDocument();
        void newDocument();
        void openDocument();
        bool saveDocument();
        bool saveDocumentAs();
        void showAbout();
        String fileTitle() const;
        bool handleEscape();
        RoseEditor::Host editorHost();

        UIWindow* mWindow = nullptr;
        AxionHost mHost;
        String mFont;
        float mWidth = 1280.0f;
        float mHeight = 800.0f;
        float mScale = 1.0f;
        bool mRebuild = true;
        TableDocument mDoc;
        std::shared_ptr<RoseEditor> mEditor;

        ConfirmKind mConfirm = ConfirmKind::None;
        PendingAction mPending = PendingAction::None;
        bool mAboutOpen = false;
        bool mCloseAllowed = false;
        int mConfirmCol = -1;
        int mConfirmTable = -1;
        int mConfirmSchema = -1;
        int mBoundTable = -1;
        String mPendingType;
        bool mResetScroll = false;
    };

}

#endif
