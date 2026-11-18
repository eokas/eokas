#ifndef _EOKAS_ROSE_AXION_INSTANCE_DESIGNER_H_
#define _EOKAS_ROSE_AXION_INSTANCE_DESIGNER_H_

#include "./Document.h"
#include "./Host.h"
#include "./InstanceKind.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>
#include <vector>

namespace eokas {
    class UIColorPicker;
    class UIView;
}

namespace eokas::RoseAxion {

    class RoseInstanceEditor_ListGrid;
    class RoseInstanceEditor_Struct;

    class RoseInstanceDesigner : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
            std::function<void(const String&)> openExternal;
            std::function<String(const String& suggested)> pickFilePath;
            std::function<String(const String& suggested)> pickFolderPath;
            std::function<bool()> ctrlDown;
            UIWidget* overlay = nullptr;
        };

        struct ValueEditState
        {
            enum class Kind { None, Option, Date, Color };
            Kind kind = Kind::None;
            FieldKind fieldKind = FieldKind::Text;
            std::vector<FieldOption> options;
            TemporalParts date;
        };

        void bind(TableDocument& doc, const Host& host);
        void setOverlay(UIWidget* overlay);
        void reset();
        void rebuild();
        float scrollY() const;
        void setScroll(float y);
        UIWidget* focusWidget() const;
        bool handleEscape();
        bool colorDropping() const;
        void sampleColorDropper(float x, float y, bool commit);
        void cancelColorDropper();
        void clearOverlayEditors();
        void placeOverlayEditors();
        void layout() override;

    private:
        void notifyRebuild();
        const String& font() const;
        float scale() const;
        void buildScalarBody(UIWidget& body, float panelW, float bodyH);
        void beginValueEdit(ValueEditState::Kind kind, const NestedField& info, const String& shown);
        void closeValueEdit();
        bool commitScalarText(const String& text);
        void commitScalarBool(bool value);
        void commitScalarLabels(const std::vector<String>& labels);

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIView* mView = nullptr;
        UIWidget* mFocus = nullptr;
        UIWidget* mOverlay = nullptr;
        UIWidget* mNameInput = nullptr;
        std::shared_ptr<RoseInstanceEditor_Struct> mStructEditor;
        std::shared_ptr<RoseInstanceEditor_ListGrid> mListGridEditor;
        ValueEditState mEdit;
        UIWidget* mPickAnchor = nullptr;
        UIWidget* mDateAnchor = nullptr;
        UIWidget* mColorAnchor = nullptr;
        UIWidget* mPickMenu = nullptr;
        UIWidget* mDatePanel = nullptr;
        UIWidget* mColorPanel = nullptr;
        UIColorPicker* mColorPicker = nullptr;
        UIWidget* mHeaderBar = nullptr;
        float mEditorTop = 0.0f;
        float mKeepY = 0.0f;
    };

}

#endif