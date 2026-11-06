#ifndef _EOKAS_UI_TIME_PICKER_H_
#define _EOKAS_UI_TIME_PICKER_H_

#include "UINumberPicker.h"
#include "UIText.h"

namespace eokas
{
    class UITimePicker : public UIWidget
    {
    public:
        bool showSeconds = true;
        bool chrome = true;
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

        std::function<void()> onTimeChanged;
        std::function<void()> onTimeCommitted;

        UITimePicker();

        static Vector2 preferredSize(float scale, bool showSeconds = true, UIDirection direction = UIDirection::Vertical);

        int hour() const;
        int minute() const;
        int second() const;
        void setTime(int hour, int minute, int second = 0);
        void setFont(const String& path, float size);

        void layout() override;
        void render(UIPrimitive& primitive) override;

    private:
        std::shared_ptr<UINumberPicker> mHour;
        std::shared_ptr<UINumberPicker> mMinute;
        std::shared_ptr<UINumberPicker> mSecond;
        std::shared_ptr<UIText> mColon0;
        std::shared_ptr<UIText> mColon1;
        int mSilence = 0;

        float scaleOf() const;
        void applyStyle();
        void paintColumn(UINumberPicker& column);
        void notifyChanged();
        void notifyCommitted();
        void bindColumn(UINumberPicker& column);
    };
}

#endif//_EOKAS_UI_TIME_PICKER_H_
