#include "UIDateTimePicker.h"
#include "../UIFont.h"
#include "../UIStroke.h"
#include <cmath>

namespace eokas
{
    namespace
    {
        float snapf(float v)
        {
            return floorf(v + 0.5f);
        }

        float maxf(float a, float b)
        {
            return a > b ? a : b;
        }
    }

    UIDateTimePicker::UIDateTimePicker()
    {
        pickable = true;
        UIWidget::color.a = 0.0f;

        mDate = std::make_shared<UIDatePicker>();
        mDate->chrome = false;
        mDate->onDateChanged = [this]()
        {
            if (mSilence == 0) this->notifyChanged();
        };
        mDate->onDateCommitted = [this]()
        {
            if (mSilence == 0) this->notifyCommitted();
        };
        this->attachChild(mDate);

        mTime = std::make_shared<UITimePicker>();
        mTime->chrome = false;
        mTime->onTimeChanged = [this]()
        {
            if (mSilence == 0) this->notifyChanged();
        };
        mTime->onTimeCommitted = [this]()
        {
            if (mSilence == 0) this->notifyCommitted();
        };
        this->attachChild(mTime);
        this->applyStyle();
    }

    Vector2 UIDateTimePicker::preferredSize(float scale, bool showSeconds, UIDirection direction)
    {
        if (!(scale > 0.0f)) scale = 1.0f;
        Vector2 date = UIDatePicker::preferredSize(scale, direction);
        Vector2 time = UITimePicker::preferredSize(scale, showSeconds, direction);
        float pad = snapf(10.0f * scale);
        float gap = snapf(12.0f * scale);
        if (direction == UIDirection::Horizontal)
        {
            return Vector2(pad * 2.0f + maxf(date.x, time.x), pad * 2.0f + date.y + gap + time.y);
        }
        return Vector2(pad * 2.0f + date.x + gap + time.x, pad * 2.0f + maxf(date.y, time.y));
    }

    int UIDateTimePicker::year() const
    {
        return mDate ? mDate->year() : 1970;
    }

    int UIDateTimePicker::month() const
    {
        return mDate ? mDate->month() : 1;
    }

    int UIDateTimePicker::day() const
    {
        return mDate ? mDate->day() : 1;
    }

    int UIDateTimePicker::hour() const
    {
        return mTime ? mTime->hour() : 0;
    }

    int UIDateTimePicker::minute() const
    {
        return mTime ? mTime->minute() : 0;
    }

    int UIDateTimePicker::second() const
    {
        return mTime ? mTime->second() : 0;
    }

    TimePoint UIDateTimePicker::value() const
    {
        return TimePoint(this->year(), this->month(), this->day(), this->hour(), this->minute(), this->second());
    }

    void UIDateTimePicker::setValue(const TimePoint& time)
    {
        this->setDateTime(time.year(), time.month(), time.date(), time.hour(), time.minute(), time.second());
    }

    void UIDateTimePicker::setDateTime(int year, int month, int day, int hour, int minute, int second)
    {
        bool changed =
            year != this->year() || month != this->month() || day != this->day() ||
            hour != this->hour() || minute != this->minute() || second != this->second();
        mSilence += 1;
        if (mDate) mDate->setDate(year, month, day);
        if (mTime) mTime->setTime(hour, minute, second);
        mSilence -= 1;
        if (changed)
        {
            this->notifyChanged();
        }
    }

    void UIDateTimePicker::setYearRange(int minYear, int maxYear)
    {
        if (mDate)
        {
            mDate->setYearRange(minYear, maxYear);
        }
    }

    void UIDateTimePicker::setFont(const String& path, float size)
    {
        fontPath = path;
        if (size > 0.0f) fontSize = size;
        this->applyStyle();
    }

    float UIDateTimePicker::scaleOf() const
    {
        return uiScale > 0.0f ? uiScale : 1.0f;
    }

    void UIDateTimePicker::applyStyle()
    {
        auto paintDate = [this](UIDatePicker& picker)
        {
            picker.direction = direction;
            picker.uiScale = this->scaleOf();
            picker.fontPath = fontPath;
            picker.fontSize = fontSize;
            picker.background = background;
            picker.field = field;
            picker.ink = ink;
            picker.muted = muted;
            picker.accent = accent;
            picker.button = button;
            picker.buttonHover = buttonHover;
            picker.buttonPress = buttonPress;
            picker.line = line;
            picker.chrome = false;
            picker.setFont(fontPath, fontSize);
        };
        auto paintTime = [this](UITimePicker& picker)
        {
            picker.direction = direction;
            picker.uiScale = this->scaleOf();
            picker.showSeconds = showSeconds;
            picker.fontPath = fontPath;
            picker.fontSize = fontSize;
            picker.background = background;
            picker.field = field;
            picker.ink = ink;
            picker.muted = muted;
            picker.accent = accent;
            picker.button = button;
            picker.buttonHover = buttonHover;
            picker.buttonPress = buttonPress;
            picker.line = line;
            picker.chrome = false;
            picker.setFont(fontPath, fontSize);
        };
        if (mDate) paintDate(*mDate);
        if (mTime) paintTime(*mTime);
    }

    void UIDateTimePicker::notifyChanged()
    {
        if (onDateTimeChanged)
        {
            onDateTimeChanged();
        }
    }

    void UIDateTimePicker::notifyCommitted()
    {
        if (onDateTimeCommitted)
        {
            onDateTimeCommitted();
        }
    }

    void UIDateTimePicker::layout()
    {
        if (!visible)
        {
            return;
        }
        this->applyStyle();
        float s = this->scaleOf();
        Vector2 dateSize = UIDatePicker::preferredSize(s, direction);
        Vector2 timeSize = UITimePicker::preferredSize(s, showSeconds, direction);
        float pad = snapf(10.0f * s);
        float gap = snapf(12.0f * s);
        float innerW = shape.size.x - pad * 2.0f;
        float innerH = shape.size.y - pad * 2.0f;
        if (innerW < 1.0f) innerW = 1.0f;
        if (innerH < 1.0f) innerH = 1.0f;

        bool stacked = direction == UIDirection::Horizontal || innerW + 0.5f < dateSize.x + gap + timeSize.x;
        if (stacked)
        {
            float width = maxf(dateSize.x, timeSize.x);
            if (width > innerW) width = innerW;
            float x = pad + (innerW - width) * 0.5f;
            if (x < pad) x = pad;
            float y = pad;
            if (mDate)
            {
                this->placeChild(*mDate, Vector2(snapf(x), snapf(y)), Vector2(width, dateSize.y));
                y += dateSize.y + gap;
            }
            if (mTime)
            {
                this->placeChild(*mTime, Vector2(snapf(x), snapf(y)), Vector2(width, timeSize.y));
            }
        }
        else
        {
            float rowH = maxf(dateSize.y, timeSize.y);
            float rowW = dateSize.x + gap + timeSize.x;
            float x = pad + (innerW - rowW) * 0.5f;
            float y = pad + (innerH - rowH) * 0.5f;
            if (x < pad) x = pad;
            if (y < pad) y = pad;
            if (mDate)
            {
                this->placeChild(*mDate, Vector2(snapf(x), snapf(y)), Vector2(dateSize.x, rowH));
                x += dateSize.x + gap;
            }
            if (mTime)
            {
                this->placeChild(*mTime, Vector2(snapf(x), snapf(y)), Vector2(timeSize.x, rowH));
            }
        }
        UIWidget::layout();
    }

    void UIDateTimePicker::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Matrix3 localToScreen = this->matrixLocalToScreen();
        primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), background);
        UIStrokeStyle border = line;
        border.thickness = this->scaleOf();
        UIStroke::border(primitive, localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), border);
        UIWidget::render(primitive);
    }
}
