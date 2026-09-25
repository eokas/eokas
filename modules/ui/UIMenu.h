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
        Color background { 0.16f, 0.16f, 0.20f, 1.0f };
        Color hoverColor { 0.28f, 0.38f, 0.58f, 1.0f };
        std::shared_ptr<UIWidget> content;

        UIMenuItem();
        void setContent(const std::shared_ptr<UIWidget>& widget);
        void setText(const String& text);
        UIText* label() const;
        void syncSize();
        void layout(const Rect& rect) override;
        void render(UIShape& shape) override;
    };

    class UIMenu : public UIWidget
    {
    public:
        UIDirection direction = UIDirection::Horizontal;
        float padding = 2.0f;
        float spacing = 0.0f;
        std::shared_ptr<UIList> list;

        UIMenu();
        void addItem(const std::shared_ptr<UIMenuItem>& item);
        void layout(const Rect& rect) override;
        void render(UIShape& shape) override;
    };
}

#endif//_EOKAS_UI_MENU_H_
