#ifndef _EOKAS_ROSE_AXION_INSTANCE_EDITOR_STRUCT_H_
#define _EOKAS_ROSE_AXION_INSTANCE_EDITOR_STRUCT_H_

#include "./Document.h"
#include "./Host.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <set>
#include <vector>

namespace eokas {
    class UIColorPicker;
    class UIView;
}

namespace eokas::RoseAxion {

    class RoseInstanceEditor_Struct : public UIWidget
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
            std::vector<NestedHop> path;
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
        const std::set<String>& collapsed() const { return mCollapsed; }
        void setCollapsed(const std::set<String>& keys) { mCollapsed = keys; }
        UIWidget* focusWidget() const { return mFocus; }
        bool handleEscape();
        bool colorDropping() const;
        void sampleColorDropper(float x, float y, bool commit);
        void cancelColorDropper();
        void clearOverlayEditors();
        void placeOverlayEditors();
        void layout() override;

    private:
        void notifyRebuild();
        void refreshUi();
        const String& font() const;
        float scale() const;
        void commitText(const std::vector<NestedHop>& path, const String& text);
        void commitBool(const std::vector<NestedHop>& path, bool value);
        void commitLabels(const std::vector<NestedHop>& path, const std::vector<String>& labels);
        void closeValueEdit();
        void beginValueEdit(ValueEditState::Kind kind, const std::vector<NestedHop>& path, const NestedField& info, const String& shown);
        bool editingField(ValueEditState::Kind kind, const std::vector<NestedHop>& path) const;

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIView* mView = nullptr;
        UIWidget* mFocus = nullptr;
        UIWidget* mPickAnchor = nullptr;
        UIWidget* mDateAnchor = nullptr;
        UIWidget* mColorAnchor = nullptr;
        UIWidget* mPickMenu = nullptr;
        UIWidget* mDatePanel = nullptr;
        UIWidget* mColorPanel = nullptr;
        UIColorPicker* mColorPicker = nullptr;
        float mKeepY = 0.0f;
        std::set<String> mCollapsed;
        ValueEditState mEdit;
        UIWidget* mOverlay = nullptr;
        std::vector<NestedHop> mFocusPath;
        u32_t mFocusCol = 0;
        bool mHasFocusTarget = false;
    };

}

#endif