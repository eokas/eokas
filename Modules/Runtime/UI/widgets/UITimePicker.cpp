#include "UITimePicker.h"
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

        int clampTime(int v, int lo, int hi)
        {
            if (v < lo) return lo;
            if (v > hi) return hi;
            return v;
        }

        String pad2(int v)
        {
            return String::format("%02d", v);
        }
    }

    UITimePicker::UITimePicker()
    {
        pickable = true;
        UIWidget::color.a = 0.0f;

        auto makeColumn = [this](int minV, int maxV)
        {
            auto column = std::make_shared<UINumberPicker>();
            column->setRange(minV, maxV);
            column->wrap = true;
            column->formatter = pad2;
            column->background.a = 0.0f;
            this->bindColumn(*column);
            this->attachChild(column);
            return column;
        };

        mHour = makeColumn(0, 23);
        mMinute = makeColumn(0, 59);
        mSecond = makeColumn(0, 59);

        auto makeColon = [this]()
        {
            auto colon = std::make_shared<UIText>();
            colon->pickable = false;
            colon->text = ":";
            this->attachChild(colon);
            return colon;
        };
        mColon0 = makeColon();
        mColon1 = makeColon();
        this->applyStyle();
    }

    Vector2 UITimePicker::preferredSize(float scale, bool showSeconds, UIDirection direction)
    {
        if (!(scale > 0.0f)) scale = 1.0f;
        Vector2 col = UINumberPicker::preferredSize(scale, direction);
        float pad = snapf(10.0f * scale);
        float gap = snapf(14.0f * scale);
        int n = showSeconds ? 3 : 2;
        if (direction == UIDirection::Horizontal)
        {
            return Vector2(pad * 2.0f + col.x, pad * 2.0f + col.y * (float)n + gap * (float)(n - 1));
        }
        return Vector2(pad * 2.0f + col.x * (float)n + gap * (float)(n - 1), pad * 2.0f + col.y);
    }

    int UITimePicker::hour() const
    {
        return mHour ? mHour->value : 0;
    }

    int UITimePicker::minute() const
    {
        return mMinute ? mMinute->value : 0;
    }

    int UITimePicker::second() const
    {
        if (!showSeconds || !mSecond)
        {
            return 0;
        }
        return mSecond->value;
    }

    void UITimePicker::setTime(int hour, int minute, int second)
    {
        hour = clampTime(hour, 0, 23);
        minute = clampTime(minute, 0, 59);
        second = clampTime(second, 0, 59);
        bool changed = hour != this->hour() || minute != this->minute() || second != this->second();
        mSilence += 1;
        if (mHour) mHour->setValue(hour);
        if (mMinute) mMinute->setValue(minute);
        if (mSecond) mSecond->setValue(second);
        mSilence -= 1;
        if (changed)
        {
            this->notifyChanged();
        }
    }

    void UITimePicker::setFont(const String& path, float size)
    {
        fontPath = path;
        if (size > 0.0f) fontSize = size;
        this->applyStyle();
    }

    float UITimePicker::scaleOf() const
    {
        return uiScale > 0.0f ? uiScale : 1.0f;
    }

    void UITimePicker::paintColumn(UINumberPicker& column)
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

    void UITimePicker::applyStyle()
    {
        if (mHour) this->paintColumn(*mHour);
        if (mMinute) this->paintColumn(*mMinute);
        if (mSecond) this->paintColumn(*mSecond);
        auto styleColon = [this](UIText* colon)
        {
            if (colon == nullptr) return;
            colon->style.fontPath = fontPath;
            colon->style.fontSize = fontSize + 4.0f * this->scaleOf();
            colon->style.color = muted;
        };
        styleColon(mColon0.get());
        styleColon(mColon1.get());
    }

    void UITimePicker::bindColumn(UINumberPicker& column)
    {
        column.onValueChanged = [this](int)
        {
            if (mSilence == 0) this->notifyChanged();
        };
        column.onValueCommitted = [this](int)
        {
            if (mSilence == 0) this->notifyCommitted();
        };
    }

    void UITimePicker::notifyChanged()
    {
        if (onTimeChanged)
        {
            onTimeChanged();
        }
    }

    void UITimePicker::notifyCommitted()
    {
        if (onTimeCommitted)
        {
            onTimeCommitted();
        }
    }

    void UITimePicker::layout()
    {
        if (!visible)
        {
            return;
        }
        this->applyStyle();
        float s = this->scaleOf();
        Vector2 col = UINumberPicker::preferredSize(s, direction);
        float pad = chrome ? snapf(10.0f * s) : 0.0f;
        float gap = snapf(14.0f * s);
        bool seconds = showSeconds;
        if (mSecond)
        {
            mSecond->visible = seconds;
            mSecond->pickable = seconds;
        }
        if (mColon1)
        {
            mColon1->visible = seconds;
        }

        auto layoutColon = [this](UIText* colon)
        {
            if (colon == nullptr || !colon->visible)
            {
                return;
            }
            colon->layout();
        };
        layoutColon(mColon0.get());
        layoutColon(mColon1.get());

        int n = seconds ? 3 : 2;
        float innerW = shape.size.x - pad * 2.0f;
        float innerH = shape.size.y - pad * 2.0f;
        if (innerW < 1.0f) innerW = 1.0f;
        if (innerH < 1.0f) innerH = 1.0f;

        auto placeSep = [&](UIText* colon, float left, float top, float right, float bottom)
        {
            if (colon == nullptr || !colon->visible) return;
            float cx = left + (right - left - colon->shape.size.x) * 0.5f;
            float cy = top + (bottom - top - colon->shape.size.y) * 0.5f;
            this->placeChild(*colon, Vector2(snapf(cx), snapf(cy)));
        };

        if (direction == UIDirection::Horizontal)
        {
            float clusterW = col.x;
            float clusterH = col.y * (float)n + gap * (float)(n - 1);
            float x = pad + (innerW - clusterW) * 0.5f;
            float y = pad + (innerH - clusterH) * 0.5f;
            if (x < pad) x = pad;
            if (y < pad) y = pad;
            if (mHour)
            {
                this->placeChild(*mHour, Vector2(snapf(x), snapf(y)), col);
                y += col.y;
            }
            float minuteTop = y + gap;
            placeSep(mColon0.get(), x, y, x + col.x, minuteTop);
            if (mMinute)
            {
                this->placeChild(*mMinute, Vector2(snapf(x), snapf(minuteTop)), col);
                y = minuteTop + col.y;
            }
            if (seconds && mSecond)
            {
                float secondTop = y + gap;
                placeSep(mColon1.get(), x, y, x + col.x, secondTop);
                this->placeChild(*mSecond, Vector2(snapf(x), snapf(secondTop)), col);
            }
        }
        else
        {
            float rowW = col.x * (float)n + gap * (float)(n - 1);
            float rowH = col.y;
            float x = pad + (innerW - rowW) * 0.5f;
            float y = pad + (innerH - rowH) * 0.5f;
            if (x < pad) x = pad;
            if (y < pad) y = pad;
            if (mHour)
            {
                this->placeChild(*mHour, Vector2(snapf(x), snapf(y)), col);
                x += col.x;
            }
            float minuteLeft = x + gap;
            placeSep(mColon0.get(), x, y, minuteLeft, y + rowH);
            float minuteRight = minuteLeft;
            if (mMinute)
            {
                this->placeChild(*mMinute, Vector2(snapf(minuteLeft), snapf(y)), col);
                minuteRight = minuteLeft + col.x;
            }
            if (seconds && mSecond)
            {
                float secondLeft = minuteRight + gap;
                placeSep(mColon1.get(), minuteRight, y, secondLeft, y + rowH);
                this->placeChild(*mSecond, Vector2(snapf(secondLeft), snapf(y)), col);
            }
        }
        UIWidget::layout();
    }

    void UITimePicker::render(UIPrimitive& primitive)
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
