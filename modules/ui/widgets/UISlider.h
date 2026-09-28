#ifndef _EOKAS_UI_SLIDER_H_
#define _EOKAS_UI_SLIDER_H_

#include "../UIWidget.h"

namespace eokas
{
    enum class SliderType
    {
        Horizontal,
        Vertical,
        Clock,
        Angular
    };

    class UISlider : public UIWidget
    {
    public:
        SliderType type = SliderType::Horizontal;
        float value = 0.0f;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        float trackThickness = 2.0f;
        float thumbSize = 16.0f;
        Color track { Color(0.24f, 0.26f, 0.32f, 1.0f) };
        Color fill { Color(0.32f, 0.44f, 0.68f, 1.0f) };
        Color thumb { Color(0.92f, 0.92f, 0.94f, 1.0f) };
        Color thumbHover { Color(0.32f, 0.44f, 0.68f, 1.0f) };
        Color thumbPressed { Color(0.18f, 0.28f, 0.50f, 1.0f) };
        std::function<void(float)> onValueChanged;

        void setValue(float v);
        void setRange(float minV, float maxV);
        void render(UIPrimitive& primitive) override;
        void triggerPointerMove(const Vector2& position, const Vector2& delta) override;
        void triggerPointerRelease() override;

    private:
        bool mTracking = false;
        float mAccum = 0.0f;
        float mLastAngle = 0.0f;

        void commitValue(float next);
        float valueT() const;
        bool ring() const;
        float pointerAngle(float x, float y) const;
        Vector2 ringPoint(float cx, float cy, float radius, float angle) const;
        float ringT(float x, float y, bool begin);
        Color thumbDrawColor() const;
        void renderLinear(UIPrimitive& primitive, const Matrix3& localToScreen, bool vertical);
        void renderRing(UIPrimitive& primitive, const Matrix3& localToScreen);
        void addArc(UIPrimitive& primitive, const Matrix3& localToScreen, float cx, float cy, float radius, float a0, float a1, const Color& color);
        void addDisc(UIPrimitive& primitive, const Matrix3& localToScreen, float cx, float cy, float radius, const Color& color);
    };
}

#endif//_EOKAS_UI_SLIDER_H_
