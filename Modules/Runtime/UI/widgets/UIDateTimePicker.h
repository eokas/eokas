#ifndef _EOKAS_UI_DATE_TIME_PICKER_H_
#define _EOKAS_UI_DATE_TIME_PICKER_H_

#include "UIDatePicker.h"
#include "UITimePicker.h"
#include "Base/Time.h"

namespace eokas
{
    class UIDateTimePicker : public UIWidget
    {
    public:
        bool showSeconds = true;
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

        std::function<void()> onDateTimeChanged;
        std::function<void()> onDateTimeCommitted;

        UIDateTimePicker();

        static Vector2 preferredSize(float scale, bool showSeconds = true, UIDirection direction = UIDirection::Vertical);

        int year() const;
        int month() const;
        int day() const;
        int hour() const;
        int minute() const;
        int second() const;
        TimePoint value() const;
        void setValue(const TimePoint& time);
        void setDateTime(int year, int month, int day, int hour, int minute, int second = 0);
        void setYearRange(int minYear, int maxYear);
        void setFont(const String& path, float size);

        void layout() override;
        void render(UIPrimitive& primitive) override;

    private:
        std::shared_ptr<UIDatePicker> mDate;
        std::shared_ptr<UITimePicker> mTime;
        int mSilence = 0;

        float scaleOf() const;
        void applyStyle();
        void notifyChanged();
        void notifyCommitted();
    };
}

#endif//_EOKAS_UI_DATE_TIME_PICKER_H_
