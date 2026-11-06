#include "UIDatePicker.h"
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

        int clampInt(int v, int lo, int hi)
        {
            if (v < lo) return lo;
            if (v > hi) return hi;
            return v;
        }

        String pad2(int v)
        {
            return String::format("%02d", v);
        }

        String pad4(int v)
        {
            return String::format("%04d", v);
        }
    }

    UIDatePicker::UIDatePicker()
    {
        pickable = true;
        UIWidget::color.a = 0.0f;

        mYear = std::make_shared<UINumberPicker>();
        mYear->setRange(minYear, maxYear);
        mYear->wrap = false;
        mYear->formatter = pad4;
        mYear->background.a = 0.0f;
        this->bindColumn(*mYear, true);
        this->attachChild(mYear);

        mMonth = std::make_shared<UINumberPicker>();
        mMonth->setRange(1, 12);
        mMonth->wrap = true;
        mMonth->formatter = pad2;
        mMonth->setValue(1);
        mMonth->background.a = 0.0f;
        this->bindColumn(*mMonth, true);
        this->attachChild(mMonth);

        mDay = std::make_shared<UINumberPicker>();
        mDay->setRange(1, 31);
        mDay->wrap = false;
        mDay->formatter = pad2;
        mDay->setValue(1);
        mDay->background.a = 0.0f;
        this->bindColumn(*mDay, false);
        this->attachChild(mDay);

        auto makeDash = [this]()
        {
            auto dash = std::make_shared<UIText>();
            dash->pickable = false;
            dash->text = "-";
            this->attachChild(dash);
            return dash;
        };
        mDash0 = makeDash();
        mDash1 = makeDash();
        this->applyStyle();
        this->syncDayRange();
    }

    Vector2 UIDatePicker::preferredSize(float scale, UIDirection direction)
    {
        if (!(scale > 0.0f)) scale = 1.0f;
        Vector2 col = UINumberPicker::preferredSize(scale, direction);
        float pad = snapf(10.0f * scale);
        float gap = snapf(14.0f * scale);
        if (direction == UIDirection::Horizontal)
        {
            return Vector2(pad * 2.0f + col.x, pad * 2.0f + col.y * 3.0f + gap * 2.0f);
        }
        float yearW = snapf(72.0f * scale);
        return Vector2(pad * 2.0f + yearW + col.x * 2.0f + gap * 2.0f, pad * 2.0f + col.y);
    }

    bool UIDatePicker::isLeapYear(int year)
    {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    int UIDatePicker::daysInMonth(int year, int month)
    {
        static const int kDays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        if (month < 1) month = 1;
        if (month > 12) month = 12;
        if (month == 2 && isLeapYear(year))
        {
            return 29;
        }
        return kDays[month];
    }

    int UIDatePicker::year() const
    {
        return mYear ? mYear->value : minYear;
    }

    int UIDatePicker::month() const
    {
        return mMonth ? mMonth->value : 1;
    }

    int UIDatePicker::day() const
    {
        return mDay ? mDay->value : 1;
    }

    void UIDatePicker::setDate(int year, int month, int day)
    {
        year = clampInt(year, minYear, maxYear);
        month = clampInt(month, 1, 12);
        day = clampInt(day, 1, daysInMonth(year, month));
        bool changed = year != this->year() || month != this->month() || day != this->day();
        mSilence += 1;
        if (mYear) mYear->setValue(year);
        if (mMonth) mMonth->setValue(month);
        this->syncDayRange();
        if (mDay) mDay->setValue(day);
        mSilence -= 1;
        if (changed)
        {
            this->notifyChanged();
        }
    }

    void UIDatePicker::setYearRange(int minY, int maxY)
    {
        if (minY > maxY)
        {
            int tmp = minY;
            minY = maxY;
            maxY = tmp;
        }
        minYear = minY;
        maxYear = maxY;
        if (mYear)
        {
            mYear->setRange(minYear, maxYear);
        }
        this->setDate(this->year(), this->month(), this->day());
    }

    void UIDatePicker::setFont(const String& path, float size)
    {
        fontPath = path;
        if (size > 0.0f) fontSize = size;
        this->applyStyle();
    }

    float UIDatePicker::scaleOf() const
    {
        return uiScale > 0.0f ? uiScale : 1.0f;
    }

    void UIDatePicker::paintColumn(UINumberPicker& column)
    {
        column.uiScale = this->scaleOf();
        column.fontPath = fontPath;
        column.fontSize = fontSize;
        column.field = field;
        column.ink = ink;
        column.muted = muted;
        column.accent = accent;
        column.button = button;
        column.buttonHover = buttonHover;
        column.buttonPress = buttonPress;
        column.line = line;
        column.background.a = 0.0f;
        column.direction = direction;
    }

    void UIDatePicker::applyStyle()
    {
        if (mYear) this->paintColumn(*mYear);
        if (mMonth) this->paintColumn(*mMonth);
        if (mDay) this->paintColumn(*mDay);
        auto styleDash = [this](UIText* dash)
        {
            if (dash == nullptr) return;
            dash->style.fontPath = fontPath;
            dash->style.fontSize = fontSize + 2.0f * this->scaleOf();
            dash->style.color = muted;
        };
        styleDash(mDash0.get());
        styleDash(mDash1.get());
    }

    void UIDatePicker::syncDayRange()
    {
        if (!mDay)
        {
            return;
        }
        int days = daysInMonth(this->year(), this->month());
        mDay->setRange(1, days);
    }

    void UIDatePicker::bindColumn(UINumberPicker& column, bool refreshDays)
    {
        column.onValueChanged = [this, refreshDays](int)
        {
            if (refreshDays)
            {
                mSilence += 1;
                this->syncDayRange();
                mSilence -= 1;
            }
            if (mSilence == 0) this->notifyChanged();
        };
        column.onValueCommitted = [this, refreshDays](int)
        {
            if (refreshDays)
            {
                mSilence += 1;
                this->syncDayRange();
                mSilence -= 1;
            }
            if (mSilence == 0) this->notifyCommitted();
        };
    }

    void UIDatePicker::notifyChanged()
    {
        if (onDateChanged)
        {
            onDateChanged();
        }
    }

    void UIDatePicker::notifyCommitted()
    {
        if (onDateCommitted)
        {
            onDateCommitted();
        }
    }

    void UIDatePicker::layout()
    {
        if (!visible)
        {
            return;
        }
        this->applyStyle();
        float s = this->scaleOf();
        Vector2 col = UINumberPicker::preferredSize(s, direction);
        Vector2 yearSize = direction == UIDirection::Horizontal ? col : Vector2(snapf(72.0f * s), col.y);
        float pad = chrome ? snapf(10.0f * s) : 0.0f;
        float gap = snapf(14.0f * s);

        if (mDash0) mDash0->layout();
        if (mDash1) mDash1->layout();

        float innerW = shape.size.x - pad * 2.0f;
        float innerH = shape.size.y - pad * 2.0f;
        if (innerW < 1.0f) innerW = 1.0f;
        if (innerH < 1.0f) innerH = 1.0f;

        auto placeSep = [&](UIText* dash, float left, float top, float right, float bottom)
        {
            if (dash == nullptr) return;
            float cx = left + (right - left - dash->shape.size.x) * 0.5f;
            float cy = top + (bottom - top - dash->shape.size.y) * 0.5f;
            this->placeChild(*dash, Vector2(snapf(cx), snapf(cy)));
        };

        if (direction == UIDirection::Horizontal)
        {
            float clusterW = col.x;
            float clusterH = col.y * 3.0f + gap * 2.0f;
            float x = pad + (innerW - clusterW) * 0.5f;
            float y = pad + (innerH - clusterH) * 0.5f;
            if (x < pad) x = pad;
            if (y < pad) y = pad;
            if (mYear)
            {
                this->placeChild(*mYear, Vector2(snapf(x), snapf(y)), col);
                y += col.y;
            }
            float monthTop = y + gap;
            placeSep(mDash0.get(), x, y, x + col.x, monthTop);
            if (mMonth)
            {
                this->placeChild(*mMonth, Vector2(snapf(x), snapf(monthTop)), col);
                y = monthTop + col.y;
            }
            float dayTop = y + gap;
            placeSep(mDash1.get(), x, y, x + col.x, dayTop);
            if (mDay)
            {
                this->placeChild(*mDay, Vector2(snapf(x), snapf(dayTop)), col);
            }
        }
        else
        {
            float rowW = yearSize.x + col.x * 2.0f + gap * 2.0f;
            float rowH = col.y;
            float x = pad + (innerW - rowW) * 0.5f;
            float y = pad + (innerH - rowH) * 0.5f;
            if (x < pad) x = pad;
            if (y < pad) y = pad;
            if (mYear)
            {
                this->placeChild(*mYear, Vector2(snapf(x), snapf(y)), yearSize);
                x += yearSize.x;
            }
            float monthLeft = x + gap;
            placeSep(mDash0.get(), x, y, monthLeft, y + rowH);
            if (mMonth)
            {
                this->placeChild(*mMonth, Vector2(snapf(monthLeft), snapf(y)), col);
                x = monthLeft + col.x;
            }
            float dayLeft = x + gap;
            placeSep(mDash1.get(), x, y, dayLeft, y + rowH);
            if (mDay)
            {
                this->placeChild(*mDay, Vector2(snapf(dayLeft), snapf(y)), col);
            }
        }
        UIWidget::layout();
    }

    void UIDatePicker::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        if (chrome)
        {
            Matrix3 localToScreen = this->matrixLocalToScreen();
            primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), background);
            UIStrokeStyle border = line;
            border.thickness = this->scaleOf();
            UIStroke::border(primitive, localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), border);
        }
        UIWidget::render(primitive);
    }
}
