#ifndef _EOKAS_UI_DROPDOWN_H_
#define _EOKAS_UI_DROPDOWN_H_

#include "UIText.h"

namespace eokas
{
    class UIDropdown;

    class UIDropdownItem : public UIWidget
    {
        friend class UIDropdown;

    public:
        int index = 0;
        float paddingX = 8.0f;
        float paddingY = 6.0f;
        Color background { 0.16f, 0.16f, 0.20f, 1.0f };
        Color hoverColor { 0.28f, 0.38f, 0.58f, 1.0f };
        Color pressedColor { 0.18f, 0.28f, 0.50f, 1.0f };
        Color selectedColor { 0.32f, 0.44f, 0.68f, 1.0f };

        void setLabel(const String& text);
        UIText* label() const;
        void render(UIShape& shape) override;
        void triggerClick() override;

    private:
        UIDropdown* owner = nullptr;
        bool selected = false;
        std::shared_ptr<UIText> mLabel;

        void layoutLabel();
    };

    class UIDropdown : public UIWidget
    {
    public:
        int value = -1;
        bool expanded = false;
        UIWidget* dropdown = nullptr;
        String placeholder;
        float paddingX = 8.0f;
        float paddingY = 6.0f;
        float borderThickness = 1.0f;
        Color background { 0.14f, 0.15f, 0.18f, 1.0f };
        Color hoverColor { 0.20f, 0.22f, 0.28f, 1.0f };
        Color pressedColor { 0.18f, 0.28f, 0.50f, 1.0f };
        Color borderColor { 0.32f, 0.44f, 0.68f, 1.0f };
        Color textColor { 0.92f, 0.92f, 0.94f, 1.0f };
        Color placeholderColor { 0.55f, 0.58f, 0.64f, 1.0f };
        Color chevronColor { 0.92f, 0.92f, 0.94f, 1.0f };
        Color popupColor { 0.16f, 0.16f, 0.20f, 1.0f };
        Color itemHoverColor { 0.28f, 0.38f, 0.58f, 1.0f };
        Color itemPressedColor { 0.18f, 0.28f, 0.50f, 1.0f };
        Color itemSelectedColor { 0.32f, 0.44f, 0.68f, 1.0f };
        std::function<void(int)> onValueChanged;

        UIDropdown();
        ~UIDropdown() override;
        void addItem(int index, const String& label);
        void removeItem(int index);
        void clearItems();
        void setValue(int index);
        void setExpanded(bool next);
        String labelOf(int index) const;
        UIText* caption() const;
        bool acceptsKeyFocus() const override { return true; }
        void render(UIShape& shape) override;
        void triggerClick() override;
        void triggerBlur() override;
        void triggerKey(UIKey key, const UIKeyMods& mods) override;

    private:
        std::shared_ptr<UIText> mCaption;
        std::vector<std::shared_ptr<UIDropdownItem>> mItems;

        UIDropdownItem* findItem(int index) const;
        int neighbor(int direction) const;
        void copyFont(UIText* text) const;
        void syncCaption();
        void syncPopup();
        void layoutCaption();
        void drawBorder(UIShape& shape, const Rect& area) const;
        void drawChevron(UIShape& shape) const;
    };
}

#endif//_EOKAS_UI_DROPDOWN_H_
