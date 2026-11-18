#ifndef _EOKAS_ROSE_AXION_INSTANCE_OUTLINER_H_
#define _EOKAS_ROSE_AXION_INSTANCE_OUTLINER_H_

#include "./Document.h"
#include "./Host.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>

namespace eokas {
    class UIView;
}

namespace eokas::RoseAxion {

    class RoseInstanceOutliner : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
            std::function<void(int)> confirmDeleteInstance;
        };

        void bind(TableDocument& doc, const Host& host);
        void setOverlay(UIWidget* overlay);
        void reset();
        void rebuild();
        bool handleEscape();
        UIWidget* focusWidget() const { return mFocus; }
        float scrollY() const;
        void setScroll(float y);
        void layout() override;

    private:
        void notifyRebuild();
        const String& font() const;
        float scale() const;
        void openInstance(u32_t index);
        void beginRename(u32_t index);
        void finishRename(const String& text);
        void cancelRename();
        void placeOverlayEditors();

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIWidget* mHeader = nullptr;
        UIWidget* mAddButton = nullptr;
        UIView* mView = nullptr;
        UIWidget* mOverlay = nullptr;
        UIWidget* mRenameAnchor = nullptr;
        UIWidget* mRenameInput = nullptr;
        UIWidget* mMenu = nullptr;
        UIWidget* mFocus = nullptr;
        int mRenameIndex = -1;
        String mRenameSeed;
        float mKeepY = 0.0f;
    };

}

#endif
