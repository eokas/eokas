#ifndef _EOKAS_ROSE_AXION_TABLE_OUTLINER_H_
#define _EOKAS_ROSE_AXION_TABLE_OUTLINER_H_

#include "./Document.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>

namespace eokas {
    class UIView;
}

namespace eokas::RoseAxion {

    class RoseTableOutliner : public UIWidget
    {
    public:
        struct Host
        {
            std::function<void()> requestRebuild;
            std::function<void(int)> confirmDeleteTable;
            String font;
            float scale = 1.0f;
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
        void openTable(u32_t index);
        void beginRename(u32_t index);
        void finishRename(const String& text);
        void cancelRename();
        void placeOverlayEditors();

        TableDocument* mDoc = nullptr;
        Host mHost;
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
