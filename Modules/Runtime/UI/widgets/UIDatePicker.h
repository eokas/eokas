#ifndef _EOKAS_UI_DATE_PICKER_H_
#define _EOKAS_UI_DATE_PICKER_H_

#include "UINumberPicker.h"
#include "UIText.h"

namespace eokas
{
    class UIDatePicker : public UIWidget
    {
    public:
        int minYear = 1970;
        int maxYear = 2100;
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

        std::function<void()> onDateChanged;
        std::function<void()> onDateCommitted;

        UIDatePicker();

        static Vector2 preferredSize(float scale, UIDirection direction = UIDirection::Vertical);
        static bool isLeapYear(int year);
        static int daysInMonth(int year, int month);

        int year() const;
        int month() const;
        int day() const;
        void setDate(int year, int month, int day);
        void setYearRange(int minY, int maxY);
        void setFont(const String& path, float size);

        void layout() override;
        void render(UIPrimitive& primitive) override;

    private:
        std::shared_ptr<UINumberPicker> mYear;
        std::shared_ptr<UINumberPicker> mMonth;
        std::shared_ptr<UINumberPicker> mDay;
        std::shared_ptr<UIText> mDash0;
        std::shared_ptr<UIText> mDash1;
        int mSilence = 0;

        float scaleOf() const;
        void applyStyle();
        void paintColumn(UINumberPicker& column);
        void syncDayRange();
        void notifyChanged();
        void notifyCommitted();
        void bindColumn(UINumberPicker& column, bool refreshDays);
    };
}

#endif//_EOKAS_UI_DATE_PICKER_H_
