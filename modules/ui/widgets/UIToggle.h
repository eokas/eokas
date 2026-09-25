#ifndef _EOKAS_UI_TOGGLE_H_
#define _EOKAS_UI_TOGGLE_H_

#include "../UIWidget.h"

namespace eokas
{
    class UIToggle : public UIWidget
    {
    public:
        bool value = false;
        Color offColor { 0.24f, 0.26f, 0.32f, 1.0f };
        Color onColor { 0.32f, 0.44f, 0.68f, 1.0f };
        Color offHoverColor { 0.32f, 0.36f, 0.44f, 1.0f };
        Color onHoverColor { 0.40f, 0.54f, 0.78f, 1.0f };
        Color offPressedColor { 0.16f, 0.18f, 0.22f, 1.0f };
        Color onPressedColor { 0.18f, 0.28f, 0.50f, 1.0f };
        Color thumbColor { 0.92f, 0.92f, 0.94f, 1.0f };
        std::function<void(bool)> onValueChanged;

        void setValue(bool v);
        void render(UIShape& shape) override;
        void triggerClick() override;

    private:
        Color capsuleColor() const;
        void addDisc(UIShape& shape, float cx, float cy, float radius, const Color& color);
    };
}

#endif//_EOKAS_UI_TOGGLE_H_
