#ifndef _EOKAS_ROSE_AXION_TABLE_DETAILS_H_
#define _EOKAS_ROSE_AXION_TABLE_DETAILS_H_

#include "./Document.h"
#include "./Host.h"
#include "./ObjectEditor.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <set>

namespace eokas::RoseAxion {

    class RoseTableDetails : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
            std::function<void(const String&)> openExternal;
            std::function<String(const String& suggested)> pickFilePath;
            std::function<String(const String& suggested)> pickFolderPath;
            std::function<bool()> ctrlDown;
            std::function<void(int, const String&)> confirmChangeType;
            std::function<void(int)> confirmDeleteColumn;
            UIWidget* overlay = nullptr;
        };

        void bind(TableDocument& doc, const Host& host);
        void setOverlay(UIWidget* overlay);
        void reset();
        void rebuild();
        void openObject(int row, int col = -1);
        void openField(int col);
        void selectField(int col);
        void close();
        void noteRowDeleted(int row);
        void noteRowInserted(int index);
        bool handleEscape();
        bool colorDropping() const;
        void sampleColorDropper(float x, float y, bool commit);
        void cancelColorDropper();
        UIWidget* focusWidget() const { return mFocus; }
        bool hasPopupEdit() const;

        void layout() override;

    private:
        void notifyRebuild();
        void captureObjectState();
        void clearObject();
        void clearTypeMenu();
        void applyFieldType(int column, const String& typeName);
        void clampState();
        const String& font() const;
        float scale() const;
        RoseObjectEditor::Host objectHost();

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIWidget* mOverlay = nullptr;
        UIWidget* mSettingsPanel = nullptr;
        RoseObjectEditor* mObjectPanel = nullptr;
        UIWidget* mHint = nullptr;
        UIWidget* mHeader = nullptr;
        UIWidget* mFocus = nullptr;
        UIWidget* mTypeAnchor = nullptr;
        UIWidget* mTypeMenu = nullptr;
        int mSettingsCol = -1;
        int mObjectRow = -1;
        int mObjectCol = -1;
        int mObjectTable = -1;
        bool mTypeOpen = false;
        float mObjectScrollY = 0.0f;
        std::set<String> mObjectCollapsed;
        RoseObjectEditor::ValueEditState mObjectEdit;
    };

}

#endif
