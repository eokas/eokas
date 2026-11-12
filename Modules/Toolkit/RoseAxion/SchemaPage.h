#ifndef _EOKAS_ROSE_AXION_SCHEMA_PAGE_H_
#define _EOKAS_ROSE_AXION_SCHEMA_PAGE_H_

#include "./Document.h"
#include "./SchemaEditor.h"
#include "./SchemaOutliner.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>

namespace eokas::RoseAxion {

    class RoseSchemaPage : public UIWidget
    {
    public:
        struct Host
        {
            std::function<void()> requestRebuild;
            std::function<void(const String&)> writeClipboard;
            std::function<String()> readClipboard;
            std::function<void(int)> confirmDeleteSchema;
            String font;
            float scale = 1.0f;
        };

        void bind(TableDocument& doc, const Host& host);
        void setOverlay(UIWidget* overlay);
        void reset();
        void resetOutliner();
        void rebuild();
        bool resizeCursorAt(float x, float y) const;
        bool handleEscape();
        UIWidget* focusWidget() const;

        void layout() override;

    private:
        const String& font() const;
        float scale() const;
        float outlinerWidth() const;
        void paneRects(float& outlinerW, float& editorX, float& editorW) const;
        void applyPaneWidth(float pixelWidth);
        void placePaneHandle();
        void attachPaneHandle();
        void commitPaneResize();
        bool hitResizePane(float screenX, float screenY) const;
        void cancelResize();
        RoseSchemaEditor::Host editorHost() const;
        RoseSchemaOutliner::Host outlinerHost() const;

        TableDocument* mDoc = nullptr;
        Host mHost;
        std::shared_ptr<RoseSchemaOutliner> mOutliner;
        std::shared_ptr<RoseSchemaEditor> mSchemaEditor;
        UIWidget* mSplit = nullptr;
        UIWidget* mSplitHandle = nullptr;
        UIWidget* mHint = nullptr;
        UIWidget* mOverlay = nullptr;
        float mKeepOutlinerY = 0.0f;
        float mKeepEditorY = 0.0f;
        float mOutlinerW = 240.0f;
        bool mResizePane = false;
        float mResizeOriginPaneW = 0.0f;
    };

}

#endif
