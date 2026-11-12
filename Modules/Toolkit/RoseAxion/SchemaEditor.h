#ifndef _EOKAS_ROSE_AXION_SCHEMA_EDITOR_H_
#define _EOKAS_ROSE_AXION_SCHEMA_EDITOR_H_

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

    class RoseSchemaEditor : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
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
        void beginSchemaNameRename();
        void finishRename(const String& text);
        void cancelRename();
        void commitMemberName(u32_t memberIndex, const String& text);
        void commitOptionName(u32_t optionIndex, const String& text);
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
        UIWidget* mColorAnchor = nullptr;
        UIWidget* mColorPanel = nullptr;
        UIWidget* mFocus = nullptr;
        int mRenameIndex = -1;
        bool mRenameSchemaName = false;
        int mTypeIndex = -2;
        int mColorIndex = -1;
        String mRenameSeed;
        float mKeepY = 0.0f;
    };

}

#endif
