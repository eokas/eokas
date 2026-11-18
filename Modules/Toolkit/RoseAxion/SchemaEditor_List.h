#ifndef _EOKAS_ROSE_AXION_SCHEMA_EDITOR_LIST_H_
#define _EOKAS_ROSE_AXION_SCHEMA_EDITOR_LIST_H_

#include "./Document.h"
#include "./Host.h"
#include "./SchemaTypeDrop.h"

#include "UI/UIWidget.h"

namespace eokas {
    class UIView;
}

namespace eokas::RoseAxion {

    class RoseSchemaEditor_List : public UIWidget
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
        void applyElementType(const String& typeName);
        void detachTypeMenu();

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIView* mView = nullptr;
        UIWidget* mFocus = nullptr;
        UIWidget* mOverlay = nullptr;
        RoseSchemaTypeMenu mTypeMenu;
        int mTypeIndex = -2;
        float mKeepY = 0.0f;
    };

}

#endif