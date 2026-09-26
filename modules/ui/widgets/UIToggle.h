#ifndef _EOKAS_UI_TOGGLE_H_
#define _EOKAS_UI_TOGGLE_H_

#include "../UIWidget.h"

namespace eokas
{
    class UIToggle : public UIWidget
    {
    public:
        bool value = false;
        Color off { Color(0.24f, 0.26f, 0.32f, 1.0f) };
        Color on { Color(0.32f, 0.44f, 0.68f, 1.0f) };
        Color offHover { Color(0.32f, 0.36f, 0.44f, 1.0f) };
        Color onHover { Color(0.40f, 0.54f, 0.78f, 1.0f) };
        Color offPressed { Color(0.16f, 0.18f, 0.22f, 1.0f) };
        Color onPressed { Color(0.18f, 0.28f, 0.50f, 1.0f) };
        Color thumb { Color(0.92f, 0.92f, 0.94f, 1.0f) };
        std::function<void(bool)> onValueChanged;

        void setValue(bool v);
        void render(UIPrimitive& primitive) override;
        void triggerClick() override;

    private:
        Color capsuleColor() const;
        void addDisc(UIPrimitive& primitive, float cx, float cy, float radius, const Color& color);
    };
}

#endif//_EOKAS_UI_TOGGLE_H_
