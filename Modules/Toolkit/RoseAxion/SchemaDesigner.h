#ifndef _EOKAS_ROSE_AXION_SCHEMA_DESIGNER_H_
#define _EOKAS_ROSE_AXION_SCHEMA_DESIGNER_H_

#include "./Document.h"
#include "./Host.h"
#include "./SchemaEditor_List.h"
#include "./SchemaEditor_Options.h"
#include "./SchemaEditor_Struct.h"

#include "UI/UIWidget.h"

#include <memory>

namespace eokas::RoseAxion {

    class RoseSchemaDesigner : public UIWidget
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
        UIWidget* focusWidget() const;
        void layout() override;

    private:
        void notifyRebuild();
        const String& font() const;
        float scale() const;
        void clearNameOverlay();
        void placeOverlayEditors();
        void beginSchemaNameRename();
        void finishRename(const String& text);
        void cancelRename();
        RoseSchemaEditor_Struct::Host structHost() const;
        RoseSchemaEditor_List::Host listHost() const;
        RoseSchemaEditor_Options::Host optionsHost() const;
        void attachTypeEditor(rose::SchemaType kind, float width, float editorY, float editorH);

        TableDocument* mDoc = nullptr;
        Host mHost;
        std::shared_ptr<RoseSchemaEditor_Struct> mEditorStruct;
        std::shared_ptr<RoseSchemaEditor_List> mEditorList;
        std::shared_ptr<RoseSchemaEditor_Options> mEditorOptions;
        UIWidget* mOverlay = nullptr;
        UIWidget* mRenameAnchor = nullptr;
        UIWidget* mRenameInput = nullptr;
        UIWidget* mFocus = nullptr;
        UIWidget* mHeaderBar = nullptr;
        bool mRenameSchemaName = false;
        String mRenameSeed;
        float mEditorTop = 0.0f;
        float mKeepY = 0.0f;
    };

}

#endif