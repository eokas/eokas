#ifndef _EOKAS_ROSE_AXION_TABLE_PAGE_H_
#define _EOKAS_ROSE_AXION_TABLE_PAGE_H_

#include "./Document.h"
#include "./TableEditor.h"
#include "./TableOutliner.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>

namespace eokas::RoseAxion {

    class RoseTablePage : public UIWidget
    {
    public:
        struct Host
        {
            std::function<void()> requestRebuild;
            std::function<void(const String&)> writeClipboard;
            std::function<String()> readClipboard;
            std::function<void(const String&)> openExternal;
            std::function<String(const String& suggested)> pickFilePath;
            std::function<String(const String& suggested)> pickFolderPath;
            std::function<bool()> ctrlDown;
            std::function<bool(UIKeyCode, bool)> handleShortcut;
            std::function<void(int, const String&)> confirmChangeType;
            std::function<void(int)> confirmDeleteColumn;
            std::function<void(int)> confirmDeleteTable;
            std::function<void(int)> confirmDeleteSchema;
            String font;
            float scale = 1.0f;
        };

        void bind(TableDocument& doc, const Host& host);
        void setOverlay(UIWidget* overlay);
        void reset();
        void resetOutliner();
        void closeSettings();
        void rebuild();
        bool resizeCursorAt(float x, float y) const;
        bool handleEscape();
        bool colorDropping() const;
        void sampleColorDropper(float x, float y, bool commit);
        void cancelColorDropper();
        float scrollX() const;
        float scrollY() const;
        void setScroll(float x, float y);
        UIWidget* focusWidget() const;

        void layout() override;

    private:
        const String& font() const;
        float scale() const;
        float outlinerWidth() const;
        void paneRects(float& outlinerW, float& tableX, float& tableW) const;
        void applyPaneWidth(float pixelWidth);
        void placePaneHandle();
        void attachPaneHandle();
        void commitPaneResize();
        bool hitResizePane(float screenX, float screenY) const;
        void cancelResize();
        RoseTableEditor::Host tableHost() const;
        RoseTableOutliner::Host outlinerHost() const;

        TableDocument* mDoc = nullptr;
        Host mHost;
        std::shared_ptr<RoseTableOutliner> mOutliner;
        std::shared_ptr<RoseTableEditor> mTableEditor;
        UIWidget* mSplit = nullptr;
        UIWidget* mSplitHandle = nullptr;
        UIWidget* mHint = nullptr;
        UIWidget* mOverlay = nullptr;
        float mKeepOutlinerY = 0.0f;
        float mOutlinerW = 240.0f;
        bool mResizePane = false;
        float mResizeOriginPaneW = 0.0f;
    };

}

#endif
