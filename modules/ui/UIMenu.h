#ifndef _EOKAS_UI_MENU_H_
#define _EOKAS_UI_MENU_H_

#include "UILayout.h"
#include "UIText.h"

namespace eokas
{
    class UIMenuItem : public UIWidget
    {
    public:
        float paddingX = 12.0f;
        float paddingY = 6.0f;
        Color background { 0.0f, 0.0f, 0.0f, 0.0f };
        Color hoverColor { 0.28f, 0.38f, 0.58f, 1.0f };
        std::shared_ptr<UIWidget> content;

        UIMenuItem();
        void setContent(const std::shared_ptr<UIWidget>& widget);
        void setText(const String& text);
        UIText* label() const;
        void syncSize();
        void render(UIShape& shape) override;
    };

    class UIMenu : public UIWidget
    {
    public:
        UILayoutDirection direction = UILayoutDirection::Horizontal;
        float padding = 4.0f;
        float spacing = 0.0f;
        std::shared_ptr<UILayout> layout;

        UIMenu();
        void addItem(const std::shared_ptr<UIMenuItem>& item);
        void render(UIShape& shape) override;
    };
}

#endif//_EOKAS_UI_MENU_H_
