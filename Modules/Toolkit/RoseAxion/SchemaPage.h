#ifndef _EOKAS_ROSE_AXION_SCHEMA_PAGE_H_
#define _EOKAS_ROSE_AXION_SCHEMA_PAGE_H_

#include "./Document.h"
#include "./Host.h"
#include "./UiKit.h"
#include "./SchemaDesigner.h"
#include "./SchemaOutliner.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>

namespace eokas::RoseAxion {

    class RoseSchemaPage : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
            std::function<void(int)> confirmDeleteSchema;
            float* sharedOutlinerW = nullptr;
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
        RoseSchemaDesigner::Host designerHost() const;
        RoseSchemaOutliner::Host outlinerHost() const;

        TableDocument* mDoc = nullptr;
        Host mHost;
        std::shared_ptr<RoseSchemaOutliner> mOutliner;
        std::shared_ptr<RoseSchemaDesigner> mDesigner;
        UIWidget* mSplit = nullptr;
        UIWidget* mSplitHandle = nullptr;
        UIWidget* mHint = nullptr;
        UIWidget* mOverlay = nullptr;
        float mKeepOutlinerY = 0.0f;
        float mKeepEditorY = 0.0f;
        float mOutlinerW = kRoseOutlinerDefaultW;
        bool mResizePane = false;
        float mResizeOriginPaneW = 0.0f;
    };

}

#endif