#ifndef _EOKAS_UI_MENU_H_
#define _EOKAS_UI_MENU_H_

#include "UIList.h"
#include "UIText.h"

namespace eokas
{
    class UIMenuItem : public UIWidget
    {
    public:
        float padding = 8.0f;
        float hoverInset = 4.0f;
        Color background { Color(0.0f, 0.0f, 0.0f, 0.0f) };
        Color hoverFill { Color(0.902f, 0.910f, 0.925f, 1.0f) };
        Color selectedFill { Color(0.890f, 0.918f, 0.953f, 1.0f) };
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
        float padding = 4.0f;
        float spacing = 0.0f;
        UIStrokeStyle border { Color(0.0f, 0.0f, 0.0f, 0.14f) };
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
