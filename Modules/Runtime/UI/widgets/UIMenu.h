#ifndef _EOKAS_UI_MENU_H_
#define _EOKAS_UI_MENU_H_

#include "UIList.h"
#include "UIText.h"

namespace eokas
{
    class UIMenuItem : public UIWidget
    {
    public:
        float paddingX = 12.0f;
        float paddingY = 2.0f;
        Color background { Color(0.16f, 0.16f, 0.20f, 1.0f) };
        Color hoverFill { Color(0.28f, 0.38f, 0.58f, 1.0f) };
        Color selectedFill { Color(0.32f, 0.44f, 0.68f, 1.0f) };
        bool selected = false;
        bool showChip = false;
        Color chip { Color(0.39f, 0.40f, 0.95f, 1.0f) };
        float chipSize = 12.0f;
        float chipGap = 8.0f;
        UIWidget::Ref content;

        UIMenuItem();
        void setContent(const UIWidget::Ref& widget);
        void setText(const String& text);
        UIText* label() const;
        void syncSize();
        void layout() override;
        void render(UIPrimitive& primitive) override;
    };

    class UIMenu : public UIWidget
    {
    public:
        UIDirection direction = UIDirection::Horizontal;
        float padding = 2.0f;
        float spacing = 0.0f;
        UIWidget* host = nullptr;
        std::shared_ptr<UIList> list;

        UIMenu();
        void addItem(const std::shared_ptr<UIMenuItem>& item);
        void layout() override;
        void render(UIPrimitive& primitive) override;
        bool handlePointerPressAnywhere(const UIPointerEvent& event) override;
    };
}

#endif//_EOKAS_UI_MENU_H_
