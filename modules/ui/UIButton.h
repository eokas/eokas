#ifndef _EOKAS_UI_BUTTON_H_
#define _EOKAS_UI_BUTTON_H_

#include "UIText.h"

namespace eokas
{
    class UIButton : public UIWidget
    {
    public:
        float paddingX = 12.0f;
        float paddingY = 8.0f;
        Color background { 0.24f, 0.26f, 0.32f, 1.0f };
        Color hoverColor { 0.32f, 0.44f, 0.68f, 1.0f };
        Color pressedColor { 0.18f, 0.28f, 0.50f, 1.0f };
        std::shared_ptr<UIWidget> content;

        UIButton();
        void setContent(const std::shared_ptr<UIWidget>& widget);
        void setText(const String& text);
        UIText* label() const;
        void render(UIShape& shape) override;
    };
}

#endif//_EOKAS_UI_BUTTON_H_
