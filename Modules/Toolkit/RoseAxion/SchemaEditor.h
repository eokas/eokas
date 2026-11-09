#ifndef _EOKAS_ROSE_AXION_SCHEMA_EDITOR_H_
#define _EOKAS_ROSE_AXION_SCHEMA_EDITOR_H_

#include "./Document.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>

namespace eokas {
    class UIView;
}

namespace eokas::RoseAxion {

    class RoseSchemaEditor : public UIWidget
    {
    public:
        struct Host
        {
            std::function<void()> requestRebuild;
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
        void beginRename(u32_t memberIndex);
        void finishRename(const String& text);
        void cancelRename();
        void applyType(const String& typeName);
        void placeOverlayEditors();

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIView* mView = nullptr;
        UIWidget* mOverlay = nullptr;
        UIWidget* mRenameAnchor = nullptr;
        UIWidget* mRenameInput = nullptr;
        UIWidget* mTypeAnchor = nullptr;
        UIWidget* mMenu = nullptr;
        UIWidget* mFocus = nullptr;
        int mRenameIndex = -1;
        int mTypeIndex = -2;
        String mRenameSeed;
        float mKeepY = 0.0f;
    };

}

#endif
