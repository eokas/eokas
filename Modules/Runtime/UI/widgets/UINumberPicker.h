#ifndef _EOKAS_UI_NUMBER_PICKER_H_
#define _EOKAS_UI_NUMBER_PICKER_H_

#include "../UIWidget.h"
#include "UIList.h"

namespace eokas
{
    class UINumberPicker : public UIWidget
    {
    public:
        int minValue = 0;
        int maxValue = 9;
        int value = 0;
        bool wrap = false;
        UIDirection direction = UIDirection::Vertical;
        float uiScale = 1.0f;
        String fontPath;
        float fontSize = 16.0f;

        Color background { 0.145f, 0.152f, 0.176f, 1.0f };
        Color field { 0.067f, 0.071f, 0.086f, 1.0f };
        Color ink { 0.925f, 0.933f, 0.953f, 1.0f };
        Color muted { 0.620f, 0.650f, 0.700f, 1.0f };
        Color accent { 0.345f, 0.545f, 0.930f, 1.0f };
        Color button { 0.227f, 0.247f, 0.306f, 1.0f };
        Color buttonHover { 0.290f, 0.325f, 0.400f, 1.0f };
        Color buttonPress { 0.165f, 0.180f, 0.227f, 1.0f };
        Color line { 0.040f, 0.043f, 0.055f, 1.0f };

        std::function<String(int)> formatter;
        std::function<void(int)> onValueChanged;
        std::function<void(int)> onValueCommitted;

        UINumberPicker();

        static Vector2 preferredSize(float scale, UIDirection direction = UIDirection::Vertical);

        void setRange(int minV, int maxV);
        void setValue(int v);
        void setFont(const String& path, float size);
        void step(int delta, bool commit = true);

        void layout() override;
        void render(UIPrimitive& primitive) override;
        bool handlePointerPress(const UIPointerEvent& event) override;
        bool handlePointerMove(const UIPointerEvent& event) override;
        bool handlePointerRelease(const UIPointerEvent& event) override;
        bool handleClick(float screenX, float screenY) override;
        bool handleWheel(float screenX, float screenY, float deltaX, float deltaY) override;
        bool handleKeyPress(const UIKeyEvent& event) override;
        void handleFocusLoss() override;
        void endActiveDrag() override;

    private:
        enum class Part
        {
            None,
            Wheel,
            Prev,
            Next
        };

        enum class Motion
        {
            Idle,
            Drag,
            Coast,
            Snap
        };

        float mScroll = 0.0f;
        float mVelocity = 0.0f;
        float mPressLocal = 0.0f;
        float mPressScroll = 0.0f;
        float mLastLocal = 0.0f;
        float mSnapFrom = 0.0f;
        float mSnapTo = 0.0f;
        bool mMoved = false;
        bool mDirty = false;
        Part mHover = Part::None;
        Part mPressPart = Part::None;
        Motion mMotion = Motion::Idle;
        std::chrono::steady_clock::time_point mLastTick {};
        std::chrono::steady_clock::time_point mLastMove {};
        std::chrono::steady_clock::time_point mSnapStart {};

        bool horizontal() const;
        float scaleOf() const;
        float itemExtent() const;
        float buttonExtent() const;
        float axisPos(const Vector2& local) const;
        float axisSize() const;
        int count() const;
        int normalize(int v) const;
        float wrapScroll(float scroll) const;
        float clampScroll(float scroll) const;
        int nearestValue(float scroll) const;
        float snapTarget(float scroll) const;
        String labelOf(int v) const;
        Part hitTest(float x, float y) const;
        Rect wheelClipScreen() const;
        void syncValue(bool notify);
        void notifyChanged();
        void notifyCommitted();
        void stopMotion();
        void beginSnap(float target);
        void finishDrag();
        void applyDelta(int delta, bool commit);
        void tickMotion();
        void drawChevron(UIPrimitive& primitive, float cx, float cy, float size, Part part, const Color& color) const;
        void drawLabel(UIPrimitive& primitive, const String& text, float cx, float cy, const Color& color) const;
    };
}

#endif//_EOKAS_UI_NUMBER_PICKER_H_
