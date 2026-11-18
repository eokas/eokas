#ifndef _EOKAS_ROSE_AXION_INSTANCE_PAGE_H_
#define _EOKAS_ROSE_AXION_INSTANCE_PAGE_H_

#include "./Document.h"
#include "./Host.h"
#include "./UiKit.h"
#include "./InstanceDesigner.h"
#include "./InstanceOutliner.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>

namespace eokas::RoseAxion {

    class RoseInstancePage : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
            std::function<void(const String&)> openExternal;
            std::function<String(const String& suggested)> pickFilePath;
            std::function<String(const String& suggested)> pickFolderPath;
            std::function<bool()> ctrlDown;
            std::function<void(int)> confirmDeleteInstance;
            std::function<UIWidget*()> overlay;
            float* sharedOutlinerW = nullptr;
        };

        void bind(TableDocument& doc, const Host& host);
        void setOverlay(UIWidget* overlay);
        void reset();
        void resetOutliner();
        void rebuild();
        bool resizeCursorAt(float x, float y) const;
        bool handleEscape();
        bool colorDropping() const;
        void sampleColorDropper(float x, float y, bool commit);
        void cancelColorDropper();
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
        RoseInstanceDesigner::Host designerHost() const;
        RoseInstanceOutliner::Host outlinerHost() const;

        TableDocument* mDoc = nullptr;
        Host mHost;
        std::shared_ptr<RoseInstanceOutliner> mOutliner;
        std::shared_ptr<RoseInstanceDesigner> mDesigner;
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