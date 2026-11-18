#ifndef _EOKAS_ROSE_AXION_SCHEMA_EDITOR_OPTIONS_H_
#define _EOKAS_ROSE_AXION_SCHEMA_EDITOR_OPTIONS_H_

#include "./Document.h"
#include "./Host.h"

#include "UI/UIWidget.h"

namespace eokas {
    class UIView;
}

namespace eokas::RoseAxion {

    class RoseSchemaEditor_Options : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
            UIWidget* overlay = nullptr;
        };

        void bind(TableDocument& doc, const Host& host);
        void setOverlay(UIWidget* overlay);
        void reset();
        void rebuild();
        float scrollY() const;
        void setScroll(float y);
        bool handleEscape();
        UIWidget* focusWidget() const { return mFocus; }
        void placeOverlayEditors();
        void layout() override;

    private:
        void notifyRebuild();
        const String& font() const;
        float scale() const;
        void commitOptionName(u32_t optionIndex, const String& text);
        void detachColorPanel();

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIView* mView = nullptr;
        UIWidget* mFocus = nullptr;
        UIWidget* mOverlay = nullptr;
        UIWidget* mColorAnchor = nullptr;
        UIWidget* mColorPanel = nullptr;
        int mRenameIndex = -1;
        int mColorIndex = -1;
        int mFocusOptionIndex = -1;
        String mRenameSeed;
        float mKeepY = 0.0f;
    };

}

#endif