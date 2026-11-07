#include "UIColorPicker.h"
#include "../UIFont.h"
#include "Native/NativeUtilities.h"

#include <cmath>
#include <cstdlib>

namespace eokas
{
    namespace
    {
        struct Metrics
        {
            float pad = 0.0f;
            float gap = 0.0f;
            float theme = 0.0f;
            float wheel = 0.0f;
            float bar = 0.0f;
            float barGap = 0.0f;
            float swatch = 0.0f;
            float hex = 0.0f;
            float cap = 0.0f;
            float option = 0.0f;
            float row = 0.0f;
            float rowGap = 0.0f;
            float group = 0.0f;
            float label = 0.0f;
            float spin = 0.0f;
            float chip = 0.0f;
            float chipGap = 0.0f;
            float small = 0.0f;
        };

        float snapf(float v)
        {
            return floorf(v + 0.5f);
        }

        float unit(float v)
        {
            if (!(v > 0.0f)) return 0.0f;
            if (v > 1.0f) return 1.0f;
            return v;
        }

        float hueOf(float v)
        {
            if (!(v > 0.0f)) return 0.0f;
            if (v > 360.0f) return 360.0f;
            return v;
        }

        float toLinear(float c)
        {
            c = unit(c);
            if (c <= 0.04045f) return c / 12.92f;
            return powf((c + 0.055f) / 1.055f, 2.4f);
        }

        float toSrgb(float c)
        {
            c = unit(c);
            if (c <= 0.0031308f) return c * 12.92f;
            return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
        }

        int byteOf(float c)
        {
            int v = (int)floorf(unit(c) * 255.0f + 0.5f);
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            return v;
        }

        bool nearColor(const Color& a, float r, float g, float b, float alpha)
        {
            const float e = 0.00001f;
            return fabsf(a.r - r) <= e && fabsf(a.g - g) <= e && fabsf(a.b - b) <= e && fabsf(a.a - alpha) <= e;
        }

        bool nearTheme(const Color& a, const Color& b)
        {
            const float e = 1.0f / 255.0f;
            return fabsf(a.r - b.r) <= e && fabsf(a.g - b.g) <= e && fabsf(a.b - b.b) <= e && fabsf(a.a - b.a) <= e;
        }

        void rgbToHsv(float r, float g, float b, float& h, float& s, float& v)
        {
            float max = r;
            if (g > max) max = g;
            if (b > max) max = b;
            float min = r;
            if (g < min) min = g;
            if (b < min) min = b;
            v = max;
            float d = max - min;
            s = max <= 0.00001f ? 0.0f : d / max;
            h = 0.0f;
            if (d <= 0.00001f) return;
            if (max == r) h = (g - b) / d + (g < b ? 6.0f : 0.0f);
            else if (max == g) h = (b - r) / d + 2.0f;
            else h = (r - g) / d + 4.0f;
            h *= 60.0f;
            if (h < 0.0f) h += 360.0f;
            if (h >= 360.0f) h = 0.0f;
        }

        void hsvToRgb(float h, float s, float v, float& r, float& g, float& b)
        {
            h = fmodf(h, 360.0f);
            if (h < 0.0f) h += 360.0f;
            float c = v * s;
            float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
            float m = v - c;
            float rp = 0.0f;
            float gp = 0.0f;
            float bp = 0.0f;
            int sector = (int)floorf(h / 60.0f);
            if (sector < 0) sector = 0;
            if (sector > 5) sector = 5;
            switch (sector)
            {
            case 0: rp = c; gp = x; bp = 0.0f; break;
            case 1: rp = x; gp = c; bp = 0.0f; break;
            case 2: rp = 0.0f; gp = c; bp = x; break;
            case 3: rp = 0.0f; gp = x; bp = c; break;
            case 4: rp = x; gp = 0.0f; bp = c; break;
            default: rp = c; gp = 0.0f; bp = x; break;
            }
            r = rp + m;
            g = gp + m;
            b = bp + m;
        }

        Color hsvColor(float h, float s, float v, float a)
        {
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            hsvToRgb(h, s, v, r, g, b);
            return Color(r, g, b, a);
        }

        Metrics metricsOf(float s)
        {
            Metrics m;
            m.pad = 10.0f * s;
            m.gap = 8.0f * s;
            m.theme = 28.0f * s;
            m.wheel = 188.0f * s;
            m.bar = 16.0f * s;
            m.barGap = 6.0f * s;
            m.swatch = 92.0f * s;
            m.hex = 22.0f * s;
            m.cap = 16.0f * s;
            m.option = 28.0f * s;
            m.row = 22.0f * s;
            m.rowGap = 4.0f * s;
            m.group = 8.0f * s;
            m.label = 16.0f * s;
            m.spin = 74.0f * s;
            m.chip = 16.0f * s;
            m.chipGap = 4.0f * s;
            m.small = 4.0f * s;
            return m;
        }

        Vector2 sizeOf(const Metrics& m)
        {
            float w = m.pad * 2.0f + m.wheel + m.gap + m.bar + m.barGap + m.bar + m.gap + m.swatch;
            float channels = 7.0f * m.row + 6.0f * m.rowGap + m.group;
            float h = m.pad * 2.0f + m.theme + m.gap + m.wheel + m.gap + m.option + m.gap + channels + m.gap + m.option;
            return Vector2(snapf(w), snapf(h));
        }

        int nibble(char c)
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        }

        bool parseFloatText(const String& text, float& out)
        {
            if (text.isEmpty()) return false;
            const char* s = text.cstr();
            char* end = nullptr;
            float v = std::strtof(s, &end);
            if (end == s) return false;
            while (*end == ' ' || *end == '\t') ++end;
            if (*end != '\0') return false;
            out = v;
            return true;
        }

        String hexDigits(const String& text)
        {
            String raw;
            for (size_t i = 0; i < text.length(); ++i)
            {
                char c = text.at(i);
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
                char buf[2] = { c, '\0' };
                raw = raw + String(buf);
            }
            if (!raw.isEmpty() && raw.at(0) == '#') raw = raw.substr(1, raw.length() - 1);
            if (raw.length() >= 2 && raw.at(0) == '0' && (raw.at(1) == 'x' || raw.at(1) == 'X'))
            {
                raw = raw.substr(2, raw.length() - 2);
            }
            return raw;
        }
    }

    UIColorPicker::UIColorPicker()
    {
        focusable = true;
        pickable = true;
        UIWidget::color.a = 0.0f;
        struct Item
        {
            Slot slot;
            const char* text;
        };
        const Item items[] = {
            { Slot::Themes, "Themes" },
            { Slot::Srgb, "sRGB" },
            { Slot::NewCap, "New" },
            { Slot::OldCap, "Old" },
            { Slot::Wheel, "Wheel" },
            { Slot::Spectrum, "Spectrum" },
            { Slot::Pick, "Pick" },
            { Slot::Ok, "OK" },
            { Slot::Cancel, "Cancel" },
            { Slot::NR, "R" },
            { Slot::NG, "G" },
            { Slot::NB, "B" },
            { Slot::NA, "A" },
            { Slot::NH, "H" },
            { Slot::NS, "S" },
            { Slot::NV, "V" },
        };
        for (int i = 0; i < (int)Slot::Count; ++i)
        {
            mText[i] = std::make_shared<UIText>();
            this->attachChild(mText[i]);
        }
        for (const Item& item : items)
        {
            mText[(int)item.slot]->text = item.text;
        }
        const float swatches[][4] = {
            { 0.00f, 0.00f, 0.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 0.86f, 0.16f, 0.16f, 1.0f },
            { 0.95f, 0.52f, 0.12f, 1.0f },
            { 0.95f, 0.84f, 0.16f, 1.0f },
            { 0.22f, 0.74f, 0.32f, 1.0f },
            { 0.14f, 0.74f, 0.84f, 1.0f },
            { 0.22f, 0.42f, 0.90f, 1.0f },
            { 0.54f, 0.26f, 0.86f, 1.0f },
            { 0.90f, 0.26f, 0.58f, 1.0f },
        };
        for (const auto& swatch : swatches)
        {
            mThemes.push_back(Color(swatch[0], swatch[1], swatch[2], swatch[3]));
        }
        mBuiltin = (int)mThemes.size();
    }

    Vector2 UIColorPicker::preferredSize(float scale)
    {
        if (!(scale > 0.0f)) scale = 1.0f;
        return sizeOf(metricsOf(scale));
    }

    void UIColorPicker::setColor(const Color& value)
    {
        this->setColor(value, true);
    }

    void UIColorPicker::setColor(const Color& value, bool resetOriginal)
    {
        float r = unit(value.r);
        float g = unit(value.g);
        float b = unit(value.b);
        float a = useAlpha ? unit(value.a) : 1.0f;
        bool changed = !nearColor(mColor, r, g, b, a);
        this->writeDisplay(r, g, b, a);
        if (resetOriginal) mOriginal = mColor;
        mDirty = false;
        if (changed) this->notifyChanged();
    }

    void UIColorPicker::setFont(const String& path, float size)
    {
        fontPath = path;
        if (size > 0.0f) fontSize = size;
        this->applyFont();
    }

    void UIColorPicker::sampleEyeDropper(float x, float y, bool commit)
    {
        (void)x;
        (void)y;
        if (!mEye) return;
        Color sample = NativeUtilities::sampleScreenColor();
        if (sample.a >= 0.0f)
        {
            float a = useAlpha ? mColor.a : 1.0f;
            this->userDisplay(sample.r, sample.g, sample.b, a);
        }
        if (!commit) return;
        mEye = false;
        if (mDirty)
        {
            mDirty = false;
            this->notifyCommitted();
        }
    }

    void UIColorPicker::cancelEyeDropper()
    {
        if (!mEye) return;
        mEye = false;
        Color before = mColor;
        mColor = mEyeColor;
        mH = mEyeH;
        mS = mEyeS;
        mV = mEyeV;
        mDirty = false;
        if (!nearColor(before, mColor.r, mColor.g, mColor.b, mColor.a)) this->notifyChanged();
    }

    float UIColorPicker::scaleOf() const
    {
        return uiScale > 0.0f ? uiScale : 1.0f;
    }

    void UIColorPicker::applyFont()
    {
        for (int i = 0; i < (int)Slot::Count; ++i)
        {
            UIText& text = *mText[i];
            text.style.fontPath = fontPath;
            text.style.fontSize = fontSize;
            text.style.color = ink;
        }
        mText[(int)Slot::Themes]->style.color = muted;
        mText[(int)Slot::NewCap]->style.color = muted;
        mText[(int)Slot::OldCap]->style.color = muted;
    }

    void UIColorPicker::syncTexts()
    {
        bool showAlpha = useAlpha;
        mText[(int)Slot::NA]->visible = showAlpha;
        mText[(int)Slot::VA]->visible = showAlpha;
        if (mEdit == kEditHex) mText[(int)Slot::Hex]->text = mEditText;
        else mText[(int)Slot::Hex]->text = this->formatHex();
        for (int channel = 0; channel < (int)Channel::Count; ++channel)
        {
            UIText& text = *mText[(int)Slot::VR + channel];
            if (mEdit == channel) text.text = mEditText;
            else text.text = this->formatChannel(channel);
        }
    }

    void UIColorPicker::placeText(Slot slot, const Box& box, float align)
    {
        UIText& text = *mText[(int)slot];
        if (!text.visible) return;
        float pad = 6.0f * mScale;
        float x = box.x + pad;
        if (align > 0.75f) x = box.x + box.w - pad - text.shape.size.x;
        else if (align > 0.25f) x = box.x + (box.w - text.shape.size.x) * 0.5f;
        if (x < box.x) x = box.x;
        float y = box.y + (box.h - text.shape.size.y) * 0.5f;
        if (y < box.y) y = box.y;
        x = snapf(x);
        y = snapf(y);
        this->placeChild(text, Vector2(x, y));
        mTextPos[(int)slot] = Vector2(x, y);
    }

    void UIColorPicker::layout()
    {
        if (!visible) return;
        mScale = this->scaleOf();
        Metrics m = metricsOf(mScale);
        this->applyFont();
        this->syncTexts();
        for (int i = 0; i < (int)Slot::Count; ++i)
        {
            if (mText[i]) mText[i]->layout();
        }

        auto box = [](float x, float y, float w, float h) {
            Box b;
            b.x = snapf(x);
            b.y = snapf(y);
            b.w = snapf(w);
            b.h = snapf(h);
            if (b.w < 0.0f) b.w = 0.0f;
            if (b.h < 0.0f) b.h = 0.0f;
            return b;
        };

        float left = m.pad;
        float right = shape.size.x - m.pad;
        float inner = right - left;
        if (inner < 1.0f) inner = 1.0f;
        float y = m.pad;
        mThemeBar = box(left, y, inner, m.theme);

        float check = 14.0f * mScale;
        if (check < 10.0f) check = 10.0f;
        float srgbW = check + 6.0f * mScale + mText[(int)Slot::Srgb]->shape.size.x;
        mSrgb = box(right - srgbW, mThemeBar.y, srgbW, mThemeBar.h);
        mCheck = box(mSrgb.x, mThemeBar.y + (mThemeBar.h - check) * 0.5f, check, check);
        {
            float tx = mCheck.x + mCheck.w + 6.0f * mScale;
            float ty = mThemeBar.y + (mThemeBar.h - mText[(int)Slot::Srgb]->shape.size.y) * 0.5f;
            this->placeChild(*mText[(int)Slot::Srgb], Vector2(snapf(tx), snapf(ty)));
            mTextPos[(int)Slot::Srgb] = Vector2(snapf(tx), snapf(ty));
        }
        this->placeText(Slot::Themes, box(left, mThemeBar.y, mText[(int)Slot::Themes]->shape.size.x + 4.0f, mThemeBar.h), 0.0f);

        float chip = m.chip;
        float chipsX = left + mText[(int)Slot::Themes]->shape.size.x + m.gap;
        float chipsRight = mSrgb.x - m.gap;
        float chipY = mThemeBar.y + (mThemeBar.h - chip) * 0.5f;
        mChips.clear();
        int count = (int)mThemes.size();
        float available = chipsRight - chipsX - chip - m.chipGap;
        int fit = 0;
        if (available >= chip) fit = (int)((available + m.chipGap) / (chip + m.chipGap));
        if (fit < 0) fit = 0;
        if (fit > count) fit = count;
        int begin = count - fit;
        float cx = chipsX;
        for (int i = begin; i < count; ++i)
        {
            Chip item;
            item.index = i;
            item.box = box(cx, chipY, chip, chip);
            mChips.push_back(item);
            cx += chip + m.chipGap;
        }
        mAdd = box(cx, chipY, chip, chip);

        y += m.theme + m.gap;
        float barsW = m.bar + m.barGap + m.bar;
        float swatchW = m.swatch;
        float graphicW = m.wheel + m.gap + barsW + m.gap + swatchW;
        float extra = inner - graphicW;
        if (extra > 0.0f)
        {
            float grow = m.swatch * 0.35f;
            if (grow > extra) grow = extra;
            swatchW += grow;
            extra -= grow;
            graphicW += grow;
        }
        float gx = left;
        if (extra > 0.0f)
        {
            gx = left + snapf(extra * 0.5f);
        }
        mWheel = box(gx, y, m.wheel, m.wheel);
        float barX = gx + m.wheel + m.gap;
        mBar0 = box(barX, y, m.bar, m.wheel);
        mBar1 = box(barX + m.bar + m.barGap, y, m.bar, m.wheel);
        float sx = mBar1.x + mBar1.w + m.gap;
        float swatchH = (m.wheel - m.cap * 2.0f - m.hex - m.small * 4.0f) * 0.5f;
        if (swatchH < 8.0f) swatchH = 8.0f;
        float sy = y;
        mNewCap = box(sx, sy, swatchW, m.cap);
        sy += m.cap + m.small;
        mNewSwatch = box(sx, sy, swatchW, swatchH);
        sy += swatchH + m.small;
        mOldCap = box(sx, sy, swatchW, m.cap);
        sy += m.cap + m.small;
        mOldSwatch = box(sx, sy, swatchW, swatchH);
        sy += swatchH + m.small;
        mHex = box(sx, sy, swatchW, m.hex);

        mCenterX = mWheel.x + mWheel.w * 0.5f;
        mCenterY = mWheel.y + mWheel.h * 0.5f;
        if (mode == UIColorPickerMode::Wheel)
        {
            mOuter = mWheel.w * 0.5f - 2.0f * mScale;
            if (mOuter < 8.0f) mOuter = 8.0f;
            mInner = mOuter * 0.77f;
            float half = mInner / 1.41421356f * 0.90f;
            mSquare = box(mCenterX - half, mCenterY - half, half * 2.0f, half * 2.0f);
        }
        else
        {
            mOuter = 0.0f;
            mInner = 0.0f;
            mSquare = mWheel;
        }

        y += m.wheel + m.gap;
        float minB = 64.0f * mScale;
        auto buttonW = [&](Slot slot) {
            float w = mText[(int)slot]->shape.size.x + 16.0f * mScale;
            if (w < minB) w = minB;
            return w;
        };
        float wheelW = buttonW(Slot::Wheel);
        float spectrumW = buttonW(Slot::Spectrum);
        float pickW = buttonW(Slot::Pick);
        mModeWheel = box(left, y, wheelW, m.option);
        mModeSpectrum = box(left + wheelW, y, spectrumW, m.option);
        mDropper = box(left + wheelW + spectrumW + m.gap, y, pickW, m.option);

        y += m.option + m.gap;
        float cursor = y;
        bool any = false;
        for (int channel = 0; channel < (int)Channel::Count; ++channel)
        {
            if (channel == (int)Channel::Alpha && !useAlpha)
            {
                mRow[channel] = Box();
                mTrack[channel] = Box();
                mSpin[channel] = Box();
                continue;
            }
            if (any) cursor += m.rowGap;
            if (channel == (int)Channel::Hue) cursor += m.group;
            mRow[channel] = box(left, cursor, inner, m.row);
            float trackH = 10.0f * mScale;
            if (trackH < 6.0f) trackH = 6.0f;
            float trackX = left + m.label + m.gap;
            float trackW = inner - m.label - m.spin - m.gap * 2.0f;
            if (trackW < 8.0f) trackW = 8.0f;
            mTrack[channel] = box(trackX, mRow[channel].y + (mRow[channel].h - trackH) * 0.5f, trackW, trackH);
            mSpin[channel] = box(left + inner - m.spin, mRow[channel].y + (mRow[channel].h - m.hex) * 0.5f, m.spin, m.hex);
            cursor += m.row;
            any = true;
        }
        y = cursor + m.gap;
        float cancelW = buttonW(Slot::Cancel);
        float okW = buttonW(Slot::Ok);
        mCancel = box(right - cancelW, y, cancelW, m.option);
        mOk = box(mCancel.x - m.gap - okW, y, okW, m.option);

        this->placeText(Slot::NewCap, mNewCap, 0.0f);
        this->placeText(Slot::OldCap, mOldCap, 0.0f);
        this->placeText(Slot::Hex, mHex, 0.0f);
        this->placeText(Slot::Wheel, mModeWheel, 0.5f);
        this->placeText(Slot::Spectrum, mModeSpectrum, 0.5f);
        this->placeText(Slot::Pick, mDropper, 0.5f);
        this->placeText(Slot::Ok, mOk, 0.5f);
        this->placeText(Slot::Cancel, mCancel, 0.5f);
        for (int channel = 0; channel < (int)Channel::Count; ++channel)
        {
            if (mRow[channel].w <= 0.0f) continue;
            Box name = box(left, mRow[channel].y, m.label, mRow[channel].h);
            this->placeText((Slot)((int)Slot::NR + channel), name, 0.5f);
            this->placeText((Slot)((int)Slot::VR + channel), mSpin[channel], 1.0f);
        }
    }

    void UIColorPicker::render(UIPrimitive& primitive)
    {
        if (!visible) return;
        Matrix3 xform = this->matrixLocalToScreen();
        Rect solid = UIFont::solidUV();
        auto fill = [&](const Box& b, const Color& c) {
            if (b.w <= 0.0f || b.h <= 0.0f || c.a <= 0.0f) return;
            primitive.addQuad(xform, b.toRect(), solid, c);
        };
        auto grad = [&](const Box& b, const Color& c0, const Color& c1, const Color& c2, const Color& c3) {
            if (b.w <= 0.0f || b.h <= 0.0f) return;
            primitive.addQuad(xform, b.toRect(), solid, c0, c1, c2, c3);
        };
        auto hgrad = [&](const Box& b, const Color& left, const Color& right) {
            grad(b, left, right, right, left);
        };
        auto vgrad = [&](const Box& b, const Color& top, const Color& bottom) {
            grad(b, top, top, bottom, bottom);
        };
        auto stroke = [&](const Box& b, const Color& c, float t) {
            if (t < 1.0f) t = 1.0f;
            if (b.w <= 0.0f || b.h <= 0.0f) return;
            fill(Box{ b.x, b.y, b.w, t }, c);
            fill(Box{ b.x, b.y + b.h - t, b.w, t }, c);
            fill(Box{ b.x, b.y, t, b.h }, c);
            fill(Box{ b.x + b.w - t, b.y, t, b.h }, c);
        };
        auto checker = [&](const Box& b) {
            float cell = 6.0f * mScale;
            if (cell < 4.0f) cell = 4.0f;
            int cols = (int)ceilf(b.w / cell);
            int rows = (int)ceilf(b.h / cell);
            if (cols < 1) cols = 1;
            if (rows < 1) rows = 1;
            if (cols > 48) cols = 48;
            if (rows > 48) rows = 48;
            float cw = b.w / (float)cols;
            float ch = b.h / (float)rows;
            Color light(0.86f, 0.86f, 0.86f, 1.0f);
            Color dark(0.58f, 0.58f, 0.58f, 1.0f);
            for (int row = 0; row < rows; ++row)
            {
                for (int col = 0; col < cols; ++col)
                {
                    fill(Box{ b.x + cw * (float)col, b.y + ch * (float)row, cw, ch }, ((col + row) & 1) ? dark : light);
                }
            }
        };
        auto annulus = [&](float cx, float cy, float inner, float outer, const Color& c) {
            if (outer <= inner || outer <= 0.0f) return;
            const int n = 28;
            for (int i = 0; i < n; ++i)
            {
                float a0 = Math::PI_MUL_2 * ((float)i / (float)n);
                float a1 = Math::PI_MUL_2 * ((float)(i + 1) / (float)n);
                Vector2 i0 = this->localToScreen(Vector2(cx + cosf(a0) * inner, cy + sinf(a0) * inner));
                Vector2 o0 = this->localToScreen(Vector2(cx + cosf(a0) * outer, cy + sinf(a0) * outer));
                Vector2 o1 = this->localToScreen(Vector2(cx + cosf(a1) * outer, cy + sinf(a1) * outer));
                Vector2 i1 = this->localToScreen(Vector2(cx + cosf(a1) * inner, cy + sinf(a1) * inner));
                primitive.addQuad(i0, o0, o1, i1, solid, c);
            }
        };
        auto selector = [&](float x, float y) {
            float rad = 6.0f * mScale;
            if (rad < 4.0f) rad = 4.0f;
            float band = 1.25f * mScale;
            if (band < 1.0f) band = 1.0f;
            annulus(x, y, rad, rad + band, Color(0.0f, 0.0f, 0.0f, 1.0f));
            annulus(x, y, rad - band, rad, Color(1.0f, 1.0f, 1.0f, 1.0f));
        };
        auto markerX = [&](const Box& track, float t) {
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            float x = track.x + t * track.w;
            Box outline { x - 2.0f, track.y - 3.0f, 4.0f, track.h + 6.0f };
            Box core { x - 1.0f, track.y - 2.0f, 2.0f, track.h + 4.0f };
            fill(outline, Color(0.0f, 0.0f, 0.0f, 1.0f));
            fill(core, Color(1.0f, 1.0f, 1.0f, 1.0f));
        };
        auto markerY = [&](const Box& bar, float t) {
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            float y = bar.y + t * bar.h;
            Box outline { bar.x - 3.0f, y - 2.0f, bar.w + 6.0f, 4.0f };
            Box core { bar.x - 2.0f, y - 1.0f, bar.w + 4.0f, 2.0f };
            fill(outline, Color(0.0f, 0.0f, 0.0f, 1.0f));
            fill(core, Color(1.0f, 1.0f, 1.0f, 1.0f));
        };
        auto displayOf = [&](float r, float g, float b) {
            if (!srgb)
            {
                r = toSrgb(r);
                g = toSrgb(g);
                b = toSrgb(b);
            }
            return Color(r, g, b, 1.0f);
        };
        auto paintHue = [&](const Box& b, bool vertical) {
            const float stops[7] = { 0.0f, 60.0f, 120.0f, 180.0f, 240.0f, 300.0f, 360.0f };
            for (int i = 0; i < 6; ++i)
            {
                Color c0 = hsvColor(stops[i], 1.0f, 1.0f, 1.0f);
                Color c1 = hsvColor(stops[i + 1], 1.0f, 1.0f, 1.0f);
                float t0 = stops[i] / 360.0f;
                float t1 = stops[i + 1] / 360.0f;
                if (vertical)
                {
                    grad(Box{ b.x, b.y + b.h * t0, b.w, b.h * (t1 - t0) }, c0, c0, c1, c1);
                }
                else
                {
                    grad(Box{ b.x + b.w * t0, b.y, b.w * (t1 - t0), b.h }, c0, c1, c1, c0);
                }
            }
        };
        auto paintAxis = [&](Axis axis, const Box& b, bool vertical) {
            fill(b, line);
            Box inner { b.x + 1.0f, b.y + 1.0f, b.w - 2.0f, b.h - 2.0f };
            if (inner.w < 1.0f) inner.w = 1.0f;
            if (inner.h < 1.0f) inner.h = 1.0f;
            if (axis == Axis::Hue)
            {
                paintHue(inner, vertical);
            }
            else if (axis == Axis::Alpha)
            {
                checker(inner);
                Color rgb(mColor.r, mColor.g, mColor.b, 1.0f);
                Color clear(mColor.r, mColor.g, mColor.b, 0.0f);
                if (vertical) vgrad(inner, rgb, clear);
                else hgrad(inner, clear, rgb);
            }
            else if (axis == Axis::Saturation)
            {
                Color full = hsvColor(mH, 1.0f, mV, 1.0f);
                Color gray(mV, mV, mV, 1.0f);
                if (vertical) vgrad(inner, full, gray);
                else hgrad(inner, gray, full);
            }
            else if (axis == Axis::Value)
            {
                Color full = hsvColor(mH, mS, 1.0f, 1.0f);
                Color black(0.0f, 0.0f, 0.0f, 1.0f);
                if (vertical) vgrad(inner, full, black);
                else hgrad(inner, black, full);
            }
            float t = 0.0f;
            if (axis == Axis::Hue) t = mH / 360.0f;
            else if (axis == Axis::Alpha) t = mColor.a;
            else if (axis == Axis::Saturation) t = mS;
            else if (axis == Axis::Value) t = mV;
            if (vertical) markerY(b, axis == Axis::Hue ? t : 1.0f - t);
            else markerX(b, t);
        };
        auto paintRgb = [&](int channel, const Box& b) {
            fill(b, line);
            Box inner { b.x + 1.0f, b.y + 1.0f, b.w - 2.0f, b.h - 2.0f };
            if (inner.w < 1.0f) inner.w = 1.0f;
            if (inner.h < 1.0f) inner.h = 1.0f;
            float r = this->channelValue((int)Channel::Red);
            float g = this->channelValue((int)Channel::Green);
            float bch = this->channelValue((int)Channel::Blue);
            int segments = srgb ? 1 : 8;
            for (int i = 0; i < segments; ++i)
            {
                float t0 = (float)i / (float)segments;
                float t1 = (float)(i + 1) / (float)segments;
                float ar = r;
                float ag = g;
                float ab = bch;
                float br = r;
                float bg = g;
                float bb = bch;
                if (channel == (int)Channel::Red) { ar = t0; br = t1; }
                else if (channel == (int)Channel::Green) { ag = t0; bg = t1; }
                else { ab = t0; bb = t1; }
                float x0 = inner.x + inner.w * t0;
                float x1 = inner.x + inner.w * t1;
                hgrad(Box{ x0, inner.y, x1 - x0, inner.h }, displayOf(ar, ag, ab), displayOf(br, bg, bb));
            }
            markerX(b, this->channelValue(channel));
        };
        auto hot = [&](const Box& b) {
            return mInside && b.contains(mLocal.x, mLocal.y);
        };
        auto held = [&](Part part) {
            return mDrag != Drag::None && mPart == part;
        };
        auto buttonFill = [&](const Box& b, Part part, bool on, bool primary) {
            bool down = held(part);
            bool over = hot(b);
            if (primary)
            {
                if (down) return this->accentPress();
                if (over && !on) return this->accentHover();
                return on ? accent : button;
            }
            if (down) return buttonPress;
            if (on) return accent;
            if (over) return buttonHover;
            return button;
        };

        fill(Box{ 0.0f, 0.0f, shape.size.x, shape.size.y }, background);
        stroke(Box{ 0.0f, 0.0f, shape.size.x, shape.size.y }, line, 1.0f);

        for (const Chip& chip : mChips)
        {
            if (chip.index < 0 || chip.index >= (int)mThemes.size()) continue;
            const Color& swatch = mThemes[(size_t)chip.index];
            if (swatch.a < 0.999f) checker(chip.box);
            fill(chip.box, swatch);
            stroke(chip.box, nearTheme(swatch, mColor) ? accent : line, 1.0f);
        }
        fill(mAdd, hot(mAdd) ? buttonHover : button);
        stroke(mAdd, line, 1.0f);
        {
            float arm = mAdd.w * 0.28f;
            float thick = mScale < 1.0f ? 1.0f : mScale;
            float mx = mAdd.x + mAdd.w * 0.5f;
            float my = mAdd.y + mAdd.h * 0.5f;
            fill(Box{ mx - arm, my - thick * 0.5f, arm * 2.0f, thick }, ink);
            fill(Box{ mx - thick * 0.5f, my - arm, thick, arm * 2.0f }, ink);
        }
        stroke(mCheck, ink, 1.0f);
        if (srgb)
        {
            float inset = 3.0f * mScale;
            if (inset < 2.0f) inset = 2.0f;
            fill(Box{ mCheck.x + inset, mCheck.y + inset, mCheck.w - inset * 2.0f, mCheck.h - inset * 2.0f }, accent);
        }

        if (mode == UIColorPickerMode::Wheel && mInner > 1.0f)
        {
            const int hole = 40;
            Vector2 center = this->localToScreen(Vector2(mCenterX, mCenterY));
            Color holeColor(0.02f, 0.02f, 0.025f, 1.0f);
            for (int i = 0; i < hole; ++i)
            {
                float a0 = Math::PI_MUL_2 * ((float)i / (float)hole);
                float a1 = Math::PI_MUL_2 * ((float)(i + 1) / (float)hole);
                Vector2 p0 = this->localToScreen(Vector2(mCenterX + cosf(a0) * mInner, mCenterY + sinf(a0) * mInner));
                Vector2 p1 = this->localToScreen(Vector2(mCenterX + cosf(a1) * mInner, mCenterY + sinf(a1) * mInner));
                primitive.addTriangle(center, p0, p1, solid, holeColor, holeColor, holeColor);
            }
            const int ring = 72;
            for (int i = 0; i < ring; ++i)
            {
                float h0 = 360.0f * (float)i / (float)ring;
                float h1 = 360.0f * (float)(i + 1) / (float)ring;
                float a0 = -Math::PI_DIV_2 + h0 / 360.0f * Math::PI_MUL_2;
                float a1 = -Math::PI_DIV_2 + h1 / 360.0f * Math::PI_MUL_2;
                Color c0 = hsvColor(h0, 1.0f, 1.0f, 1.0f);
                Color c1 = hsvColor(h1, 1.0f, 1.0f, 1.0f);
                Vector2 i0 = this->localToScreen(Vector2(mCenterX + cosf(a0) * mInner, mCenterY + sinf(a0) * mInner));
                Vector2 o0 = this->localToScreen(Vector2(mCenterX + cosf(a0) * mOuter, mCenterY + sinf(a0) * mOuter));
                Vector2 o1 = this->localToScreen(Vector2(mCenterX + cosf(a1) * mOuter, mCenterY + sinf(a1) * mOuter));
                Vector2 i1 = this->localToScreen(Vector2(mCenterX + cosf(a1) * mInner, mCenterY + sinf(a1) * mInner));
                primitive.addQuad(i0, o0, o1, i1, solid, c0, c0, c1, c1);
            }
            Color pure = hsvColor(mH, 1.0f, 1.0f, 1.0f);
            grad(mSquare, Color(1, 1, 1, 1), pure, Color(0, 0, 0, 1), Color(0, 0, 0, 1));
            stroke(mSquare, line, 1.0f);
            float ang = -Math::PI_DIV_2 + (mH / 360.0f) * Math::PI_MUL_2;
            float mid = (mInner + mOuter) * 0.5f;
            selector(mCenterX + cosf(ang) * mid, mCenterY + sinf(ang) * mid);
            selector(mSquare.x + mS * mSquare.w, mSquare.y + (1.0f - mV) * mSquare.h);
        }
        else
        {
            fill(mSquare, line);
            Box inner { mSquare.x + 1.0f, mSquare.y + 1.0f, mSquare.w - 2.0f, mSquare.h - 2.0f };
            Color gray(mV, mV, mV, 1.0f);
            for (int i = 0; i < 6; ++i)
            {
                float t0 = (float)i / 6.0f;
                float t1 = (float)(i + 1) / 6.0f;
                Color left = hsvColor(60.0f * (float)i, 1.0f, mV, 1.0f);
                Color right = hsvColor(60.0f * (float)(i + 1), 1.0f, mV, 1.0f);
                grad(Box{ inner.x + inner.w * t0, inner.y, inner.w * (t1 - t0), inner.h }, left, right, gray, gray);
            }
            selector(mSquare.x + (mH / 360.0f) * mSquare.w, mSquare.y + (1.0f - mS) * mSquare.h);
        }

        paintAxis(this->barAxis(0), mBar0, true);
        paintAxis(this->barAxis(1), mBar1, true);

        if (mNewSwatch.w > 0.0f)
        {
            if (mColor.a < 0.999f) checker(mNewSwatch);
            fill(mNewSwatch, mColor);
            stroke(mNewSwatch, line, 1.0f);
        }
        if (mOldSwatch.w > 0.0f)
        {
            if (mOriginal.a < 0.999f) checker(mOldSwatch);
            fill(mOldSwatch, mOriginal);
            stroke(mOldSwatch, line, 1.0f);
        }

        auto fieldFill = [&](const Box& b, bool editing) {
            fill(b, field);
            stroke(b, editing ? accent : line, 1.0f);
        };
        fieldFill(mHex, mEdit == kEditHex);
        for (int channel = 0; channel < (int)Channel::Count; ++channel)
        {
            if (mRow[channel].w <= 0.0f) continue;
            if (channel <= (int)Channel::Blue) paintRgb(channel, mTrack[channel]);
            else if (channel == (int)Channel::Alpha) paintAxis(Axis::Alpha, mTrack[channel], false);
            else if (channel == (int)Channel::Hue) paintAxis(Axis::Hue, mTrack[channel], false);
            else if (channel == (int)Channel::Saturation) paintAxis(Axis::Saturation, mTrack[channel], false);
            else paintAxis(Axis::Value, mTrack[channel], false);
            fieldFill(mSpin[channel], mEdit == channel);
        }

        fill(mModeWheel, buttonFill(mModeWheel, Part::ModeWheel, mode == UIColorPickerMode::Wheel, false));
        fill(mModeSpectrum, buttonFill(mModeSpectrum, Part::ModeSpectrum, mode == UIColorPickerMode::Spectrum, false));
        stroke(mModeWheel, line, 1.0f);
        stroke(mModeSpectrum, line, 1.0f);
        fill(mDropper, buttonFill(mDropper, Part::Dropper, mEye, false));
        stroke(mDropper, line, 1.0f);
        fill(mOk, buttonFill(mOk, Part::Ok, true, true));
        stroke(mOk, line, 1.0f);
        fill(mCancel, buttonFill(mCancel, Part::Cancel, false, false));
        stroke(mCancel, line, 1.0f);

        if (mEdit != kEditNone)
        {
            Slot slot = this->editSlot();
            if ((int)slot < (int)Slot::Count)
            {
                Vector2 pos = mTextPos[(int)slot];
                float textH = mText[(int)slot]->shape.size.y;
                if (mAnchor != mCaret && textH > 0.0f)
                {
                    size_t a = mAnchor < mCaret ? mAnchor : mCaret;
                    size_t b = mAnchor > mCaret ? mAnchor : mCaret;
                    float x0 = pos.x + this->textWidth(mEditText, a);
                    float x1 = pos.x + this->textWidth(mEditText, b);
                    Color sel = accent;
                    sel.a = 0.45f;
                    fill(Box{ x0, pos.y, x1 - x0, textH }, sel);
                }
            }
        }

        UIWidget::render(primitive);

        if (mEdit != kEditNone && mAnchor == mCaret)
        {
            Slot slot = this->editSlot();
            if ((int)slot < (int)Slot::Count)
            {
                float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - mBlink).count();
                if (seconds < 0.0f) seconds = 0.0f;
                if (fmodf(seconds, 1.0f) < 0.5f)
                {
                    Vector2 pos = mTextPos[(int)slot];
                    float textH = mText[(int)slot]->shape.size.y;
                    if (textH < 1.0f) textH = fontSize;
                    float x = pos.x + this->textWidth(mEditText, mCaret);
                    fill(Box{ x, pos.y, 1.0f, textH }, ink);
                }
            }
        }
    }

    bool UIColorPicker::handlePointerEnter(const UIPointerEvent& event)
    {
        mInside = true;
        this->remember(event.position.x, event.position.y);
        return UIWidget::handlePointerEnter(event);
    }

    bool UIColorPicker::handlePointerLeave(const UIPointerEvent& event)
    {
        mInside = false;
        return UIWidget::handlePointerLeave(event);
    }

    bool UIColorPicker::handlePointerMove(const UIPointerEvent& event)
    {
        bool handled = UIWidget::handlePointerMove(event);
        this->remember(event.position.x, event.position.y);
        if (mDrag == Drag::None) return handled;
        float dx = mLocal.x - mPressX;
        float dy = mLocal.y - mPressY;
        float slop = 4.0f * mScale;
        if (slop < 3.0f) slop = 3.0f;
        if (dx * dx + dy * dy > slop * slop) mMoved = true;
        if (mDrag == Drag::Click && mPart == Part::Spin && mMoved) mDrag = Drag::Scrub;
        if (mDrag == Drag::Wheel) this->applyGraphic(mLocal.x, mLocal.y);
        else if (mDrag == Drag::Bar0) this->applyBar(0, mLocal.x, mLocal.y);
        else if (mDrag == Drag::Bar1) this->applyBar(1, mLocal.x, mLocal.y);
        else if (mDrag == Drag::Track) this->applyTrack(mIndex, mLocal.x, mLocal.y);
        else if (mDrag == Drag::Scrub) this->scrub(mLocal.x);
        else if (mDrag == Drag::Text)
        {
            mCaret = this->indexAt(mLocal.x);
            mAnchor = mMoved ? mPressCaret : mCaret;
            mBlink = std::chrono::steady_clock::now();
        }
        return true;
    }

    bool UIColorPicker::handlePointerPress(const UIPointerEvent& event)
    {
        UIWidget::handlePointerPress(event);
        this->remember(event.position.x, event.position.y);
        if (event.pointerCode == UIPointerCode::Right)
        {
            Hit hit = this->hitTest(mLocal.x, mLocal.y);
            if (hit.part == Part::Chip && hit.index >= mBuiltin && hit.index < (int)mThemes.size())
            {
                mThemes.erase(mThemes.begin() + hit.index);
            }
            return true;
        }
        if (event.pointerCode != UIPointerCode::Left) return true;

        Hit hit = this->hitTest(mLocal.x, mLocal.y);
        if (!this->sameField(hit)) this->commitEdit();
        mMoved = false;
        mPressX = mLocal.x;
        mPressY = mLocal.y;
        mPart = hit.part;
        mIndex = hit.index;
        mDrag = Drag::None;

        if (this->sameField(hit) && (hit.part == Part::Spin || hit.part == Part::Hex))
        {
            mDrag = Drag::Text;
            mPressCaret = this->indexAt(mLocal.x);
            mCaret = mPressCaret;
            mAnchor = mPressCaret;
            mBlink = std::chrono::steady_clock::now();
            return true;
        }
        if (hit.part == Part::None) return true;

        if (hit.part == Part::Wheel)
        {
            mDrag = Drag::Wheel;
            mWheelSV = mode == UIColorPickerMode::Spectrum || mSquare.contains(mLocal.x, mLocal.y);
            this->applyGraphic(mLocal.x, mLocal.y);
        }
        else if (hit.part == Part::Bar0 || hit.part == Part::Bar1)
        {
            mDrag = hit.part == Part::Bar0 ? Drag::Bar0 : Drag::Bar1;
            this->applyBar(hit.part == Part::Bar0 ? 0 : 1, mLocal.x, mLocal.y);
        }
        else if (hit.part == Part::Track)
        {
            mDrag = Drag::Track;
            this->applyTrack(hit.index, mLocal.x, mLocal.y);
        }
        else if (hit.part == Part::Spin)
        {
            mDrag = Drag::Click;
            mPressNumber = this->channelValue(hit.index);
        }
        else if (hit.part == Part::NewSwatch || hit.part == Part::OldSwatch)
        {
            mDrag = Drag::Swatch;
            mDragColor = hit.part == Part::NewSwatch ? mColor : mOriginal;
        }
        else
        {
            mDrag = Drag::Click;
        }
        return true;
    }

    bool UIColorPicker::handlePointerRelease(const UIPointerEvent& event)
    {
        if (event.pointerCode == UIPointerCode::Left) this->finishPointer(event.position.x, event.position.y, true);
        mDrag = Drag::None;
        return UIWidget::handlePointerRelease(event);
    }

    bool UIColorPicker::handleWheel(float screenX, float screenY, float deltaX, float deltaY)
    {
        Vector2 local = this->screenToLocal(Vector2(screenX, screenY));
        if (!this->contains(local)) return false;
        Hit hit = this->hitTest(local.x, local.y);
        int channel = this->channelOf(hit);
        if (channel < 0) return false;
        float notches = 0.0f;
        if (fabsf(deltaY) >= fabsf(deltaX)) notches = -deltaY / 48.0f;
        else notches = deltaX / 48.0f;
        if (fabsf(notches) < 0.01f) return false;
        if (mEdit != kEditNone) this->commitEdit();
        this->stepChannel(channel, notches);
        return true;
    }

    bool UIColorPicker::handleKeyPress(const UIKeyEvent& event)
    {
        if (event.key == UIKeyCode::Escape)
        {
            if (mEye)
            {
                this->cancelEyeDropper();
                return true;
            }
            if (mEdit != kEditNone)
            {
                this->cancelEdit();
                return true;
            }
            return false;
        }
        if (mEdit == kEditNone)
        {
            if (event.key == UIKeyCode::Tab)
            {
                this->beginEdit(kEditHex, true);
                return true;
            }
            return false;
        }

        auto live = [this]() {
            if (mEdit == kEditHex) this->applyHex(mEditText, false);
            else if (mEdit >= 0) this->applyNumber(mEdit, mEditText, false);
            mBlink = std::chrono::steady_clock::now();
        };
        auto fields = [this](int* out, int& count) {
            count = 0;
            out[count++] = kEditHex;
            for (int channel = 0; channel < (int)Channel::Count; ++channel)
            {
                if (channel == (int)Channel::Alpha && !useAlpha) continue;
                out[count++] = channel;
            }
        };

        if (event.key == UIKeyCode::Tab)
        {
            int list[8];
            int count = 0;
            fields(list, count);
            int pos = 0;
            for (int i = 0; i < count; ++i) if (list[i] == mEdit) pos = i;
            int step = event.shift ? count - 1 : 1;
            int next = list[(pos + step) % count];
            this->commitEdit();
            this->beginEdit(next, true);
            return true;
        }
        if (event.key == UIKeyCode::Enter || event.key == UIKeyCode::KeypadEnter)
        {
            this->commitEdit();
            return true;
        }
        if (event.key == UIKeyCode::Left) { this->moveCaret(-1, event.shift); return true; }
        if (event.key == UIKeyCode::Right) { this->moveCaret(1, event.shift); return true; }
        if (event.key == UIKeyCode::Home)
        {
            mCaret = 0;
            if (!event.shift) mAnchor = mCaret;
            mBlink = std::chrono::steady_clock::now();
            return true;
        }
        if (event.key == UIKeyCode::End)
        {
            mCaret = mEditText.length();
            if (!event.shift) mAnchor = mCaret;
            mBlink = std::chrono::steady_clock::now();
            return true;
        }
        if (event.key == UIKeyCode::Up || event.key == UIKeyCode::Down)
        {
            int which = mEdit;
            float dir = event.key == UIKeyCode::Up ? 1.0f : -1.0f;
            if (which == kEditHex) this->userHsv(mH, mS, unit(mV + dir * 0.01f), mColor.a);
            else this->setChannel(which, this->channelValue(which) + dir * (which == (int)Channel::Hue ? 1.0f : 0.01f));
            this->beginEdit(which, false);
            return true;
        }
        if (event.ctrl && (event.key == UIKeyCode::A))
        {
            mAnchor = 0;
            mCaret = mEditText.length();
            mBlink = std::chrono::steady_clock::now();
            return true;
        }
        if (event.ctrl && (event.key == UIKeyCode::C || event.key == UIKeyCode::X))
        {
            size_t a = mAnchor < mCaret ? mAnchor : mCaret;
            size_t b = mAnchor > mCaret ? mAnchor : mCaret;
            String selected = a == b ? mEditText : mEditText.substr(a, b - a);
            if (writeClipboard) writeClipboard(selected);
            if (event.key == UIKeyCode::X && a != b)
            {
                this->eraseSelection();
                live();
            }
            return true;
        }
        if (event.ctrl && event.key == UIKeyCode::V)
        {
            if (readClipboard) this->insertEdit(readClipboard());
            return true;
        }
        if (event.key == UIKeyCode::Backspace || event.key == UIKeyCode::Delete)
        {
            if (mAnchor != mCaret) this->eraseSelection();
            else if (event.key == UIKeyCode::Backspace && mCaret > 0)
            {
                size_t a = mCaret - 1;
                String left = a > 0 ? mEditText.substr(0, a) : String();
                String right = mCaret < mEditText.length() ? mEditText.substr(mCaret, mEditText.length() - mCaret) : String();
                mEditText = left + right;
                mCaret = mAnchor = a;
            }
            else if (event.key == UIKeyCode::Delete && mCaret < mEditText.length())
            {
                String left = mCaret > 0 ? mEditText.substr(0, mCaret) : String();
                String right = mEditText.substr(mCaret + 1, mEditText.length() - mCaret - 1);
                mEditText = left + right;
            }
            live();
            return true;
        }
        return true;
    }

    bool UIColorPicker::handleKeyChar(uint32_t character)
    {
        if (mEdit == kEditNone || character < 32 || character > 126) return false;
        char buf[2] = { (char)character, '\0' };
        this->insertEdit(String(buf));
        return true;
    }

    void UIColorPicker::handleFocusLoss()
    {
        this->commitEdit();
        if (mEye) this->cancelEyeDropper();
        UIWidget::handleFocusLoss();
    }

    UIColorPicker::Hit UIColorPicker::hitTest(float x, float y) const
    {
        Hit hit;
        for (const Chip& chip : mChips)
        {
            if (!chip.box.contains(x, y)) continue;
            hit.part = Part::Chip;
            hit.index = chip.index;
            return hit;
        }
        if (mAdd.contains(x, y)) { hit.part = Part::Add; return hit; }
        if (mSrgb.contains(x, y)) { hit.part = Part::Srgb; return hit; }
        if (mOk.contains(x, y)) { hit.part = Part::Ok; return hit; }
        if (mCancel.contains(x, y)) { hit.part = Part::Cancel; return hit; }
        if (mModeWheel.contains(x, y)) { hit.part = Part::ModeWheel; return hit; }
        if (mModeSpectrum.contains(x, y)) { hit.part = Part::ModeSpectrum; return hit; }
        if (mDropper.contains(x, y)) { hit.part = Part::Dropper; return hit; }
        if (mHex.contains(x, y)) { hit.part = Part::Hex; return hit; }
        if (mNewSwatch.contains(x, y)) { hit.part = Part::NewSwatch; return hit; }
        if (mOldSwatch.contains(x, y)) { hit.part = Part::OldSwatch; return hit; }
        for (int channel = 0; channel < (int)Channel::Count; ++channel)
        {
            if (!mSpin[channel].contains(x, y)) continue;
            hit.part = Part::Spin;
            hit.index = channel;
            return hit;
        }
        for (int channel = 0; channel < (int)Channel::Count; ++channel)
        {
            if (mRow[channel].w <= 0.0f) continue;
            if (y < mRow[channel].y || y >= mRow[channel].y + mRow[channel].h) continue;
            if (x < mTrack[channel].x || x >= mTrack[channel].x + mTrack[channel].w) continue;
            hit.part = Part::Track;
            hit.index = channel;
            return hit;
        }
        Box bar0 = mBar0;
        Box bar1 = mBar1;
        float slop = 3.0f * mScale;
        bar0.x -= slop;
        bar0.w += slop * 2.0f;
        bar1.x -= slop;
        bar1.w += slop * 2.0f;
        if (bar0.contains(x, y)) { hit.part = Part::Bar0; return hit; }
        if (bar1.contains(x, y)) { hit.part = Part::Bar1; return hit; }
        if (mWheel.contains(x, y)) { hit.part = Part::Wheel; return hit; }
        return hit;
    }

    bool UIColorPicker::sameField(const Hit& hit) const
    {
        if (mEdit == kEditHex) return hit.part == Part::Hex;
        if (mEdit >= 0) return hit.part == Part::Spin && hit.index == mEdit;
        return false;
    }

    int UIColorPicker::channelOf(const Hit& hit) const
    {
        if (hit.part == Part::Track || hit.part == Part::Spin) return hit.index;
        if (hit.part == Part::Hex || hit.part == Part::Wheel) return (int)Channel::Value;
        if (hit.part == Part::Bar0 || hit.part == Part::Bar1)
        {
            Axis axis = this->barAxis(hit.part == Part::Bar0 ? 0 : 1);
            if (axis == Axis::Saturation) return (int)Channel::Saturation;
            if (axis == Axis::Value) return (int)Channel::Value;
            if (axis == Axis::Hue) return (int)Channel::Hue;
            if (axis == Axis::Alpha) return (int)Channel::Alpha;
        }
        return -1;
    }

    UIColorPicker::Axis UIColorPicker::barAxis(int index) const
    {
        if (mode == UIColorPickerMode::Wheel) return index == 0 ? Axis::Saturation : Axis::Value;
        if (index == 0) return Axis::Value;
        return useAlpha ? Axis::Alpha : Axis::Hue;
    }

    float UIColorPicker::channelValue(int channel) const
    {
        if (channel == (int)Channel::Red) return srgb ? mColor.r : toLinear(mColor.r);
        if (channel == (int)Channel::Green) return srgb ? mColor.g : toLinear(mColor.g);
        if (channel == (int)Channel::Blue) return srgb ? mColor.b : toLinear(mColor.b);
        if (channel == (int)Channel::Alpha) return mColor.a;
        if (channel == (int)Channel::Hue) return mH;
        if (channel == (int)Channel::Saturation) return mS;
        if (channel == (int)Channel::Value) return mV;
        return 0.0f;
    }

    void UIColorPicker::setChannel(int channel, float value)
    {
        if (channel == (int)Channel::Alpha)
        {
            float a = useAlpha ? unit(value) : 1.0f;
            bool changed = fabsf(mColor.a - a) > 0.00001f;
            mColor.a = a;
            this->mark(changed);
            return;
        }
        if (channel == (int)Channel::Hue || channel == (int)Channel::Saturation || channel == (int)Channel::Value)
        {
            float h = mH;
            float s = mS;
            float v = mV;
            if (channel == (int)Channel::Hue) h = hueOf(value);
            else if (channel == (int)Channel::Saturation) s = unit(value);
            else v = unit(value);
            this->userHsv(h, s, v, mColor.a);
            return;
        }
        float r = this->channelValue((int)Channel::Red);
        float g = this->channelValue((int)Channel::Green);
        float b = this->channelValue((int)Channel::Blue);
        value = unit(value);
        if (channel == (int)Channel::Red) r = value;
        else if (channel == (int)Channel::Green) g = value;
        else b = value;
        if (!srgb)
        {
            r = toSrgb(r);
            g = toSrgb(g);
            b = toSrgb(b);
        }
        this->userDisplay(r, g, b, mColor.a);
    }

    void UIColorPicker::writeDisplay(float r, float g, float b, float a)
    {
        r = unit(r);
        g = unit(g);
        b = unit(b);
        a = useAlpha ? unit(a) : 1.0f;
        float h = 0.0f;
        float s = 0.0f;
        float v = 0.0f;
        rgbToHsv(r, g, b, h, s, v);
        if (s < 0.0001f || v < 0.0001f) h = mH;
        else if (h < 0.001f && mH > 359.0f) h = mH;
        mH = h;
        mS = s;
        mV = v;
        mColor = Color(r, g, b, a);
    }

    void UIColorPicker::writeHsv(float h, float s, float v, float a)
    {
        h = hueOf(h);
        s = unit(s);
        v = unit(v);
        a = useAlpha ? unit(a) : 1.0f;
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        hsvToRgb(h, s, v, r, g, b);
        mH = h;
        mS = s;
        mV = v;
        mColor = Color(r, g, b, a);
    }

    void UIColorPicker::userDisplay(float r, float g, float b, float a)
    {
        r = unit(r);
        g = unit(g);
        b = unit(b);
        a = useAlpha ? unit(a) : 1.0f;
        bool changed = !nearColor(mColor, r, g, b, a);
        this->writeDisplay(r, g, b, a);
        this->mark(changed);
    }

    void UIColorPicker::userHsv(float h, float s, float v, float a)
    {
        h = hueOf(h);
        s = unit(s);
        v = unit(v);
        a = useAlpha ? unit(a) : 1.0f;
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        hsvToRgb(h, s, v, r, g, b);
        bool changed = !nearColor(mColor, r, g, b, a);
        this->writeHsv(h, s, v, a);
        this->mark(changed);
    }

    void UIColorPicker::applyGraphic(float x, float y)
    {
        float a = mColor.a;
        if (mWheelSV)
        {
            float sw = mSquare.w > 1.0f ? mSquare.w : 1.0f;
            float sh = mSquare.h > 1.0f ? mSquare.h : 1.0f;
            float lx = Math::clamp(x, mSquare.x, mSquare.x + mSquare.w);
            float ly = Math::clamp(y, mSquare.y, mSquare.y + mSquare.h);
            if (mode == UIColorPickerMode::Spectrum)
            {
                float h = (lx - mSquare.x) / sw * 360.0f;
                float s = 1.0f - (ly - mSquare.y) / sh;
                this->userHsv(h, s, mV, a);
            }
            else
            {
                float s = (lx - mSquare.x) / sw;
                float v = 1.0f - (ly - mSquare.y) / sh;
                this->userHsv(mH, s, v, a);
            }
            return;
        }
        float ang = atan2f(y - mCenterY, x - mCenterX);
        float hue = (ang + Math::PI_DIV_2) / Math::PI_MUL_2 * 360.0f;
        if (hue < 0.0f) hue += 360.0f;
        if (hue >= 360.0f) hue -= 360.0f;
        this->userHsv(hue, mS, mV, a);
    }

    void UIColorPicker::applyBar(int index, float x, float y)
    {
        (void)x;
        const Box& bar = index == 0 ? mBar0 : mBar1;
        float span = bar.h > 1.0f ? bar.h : 1.0f;
        float t = (y - bar.y) / span;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        Axis axis = this->barAxis(index);
        if (axis == Axis::Hue) this->setChannel((int)Channel::Hue, t * 360.0f);
        else if (axis == Axis::Alpha) this->setChannel((int)Channel::Alpha, 1.0f - t);
        else if (axis == Axis::Saturation) this->setChannel((int)Channel::Saturation, 1.0f - t);
        else if (axis == Axis::Value) this->setChannel((int)Channel::Value, 1.0f - t);
    }

    void UIColorPicker::applyTrack(int channel, float x, float y)
    {
        (void)y;
        if (channel < 0 || channel >= (int)Channel::Count) return;
        const Box& track = mTrack[channel];
        float span = track.w > 1.0f ? track.w : 1.0f;
        float t = (x - track.x) / span;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        if (channel == (int)Channel::Hue) this->setChannel(channel, t * 360.0f);
        else this->setChannel(channel, t);
    }

    void UIColorPicker::scrub(float x)
    {
        if (mIndex < 0) return;
        float speed = mIndex == (int)Channel::Hue ? 1.0f : 0.005f;
        this->setChannel(mIndex, mPressNumber + (x - mPressX) * speed);
    }

    void UIColorPicker::addTheme(const Color& value)
    {
        if (!mThemes.empty() && nearTheme(mThemes.back(), value)) return;
        if (mBuiltin >= 0 && (int)mThemes.size() >= mBuiltin + 14 && mBuiltin < (int)mThemes.size())
        {
            mThemes.erase(mThemes.begin() + mBuiltin);
        }
        mThemes.push_back(value);
    }

    void UIColorPicker::activate(Part part, int index)
    {
        if (part == Part::Srgb) srgb = !srgb;
        else if (part == Part::ModeWheel) mode = UIColorPickerMode::Wheel;
        else if (part == Part::ModeSpectrum) mode = UIColorPickerMode::Spectrum;
        else if (part == Part::Dropper)
        {
            if (mEye)
            {
                this->cancelEyeDropper();
                return;
            }
            mEye = true;
            mEyeColor = mColor;
            mEyeH = mH;
            mEyeS = mS;
            mEyeV = mV;
        }
        else if (part == Part::Ok)
        {
            mOriginal = mColor;
            mDirty = false;
            this->notifyCommitted();
        }
        else if (part == Part::Cancel)
        {
            bool changed = !nearColor(mColor, mOriginal.r, mOriginal.g, mOriginal.b, mOriginal.a);
            this->writeDisplay(mOriginal.r, mOriginal.g, mOriginal.b, mOriginal.a);
            mDirty = false;
            if (changed) this->notifyChanged();
            if (onCancelled && !mSilence)
            {
                mSilence = true;
                onCancelled();
                mSilence = false;
            }
        }
        else if (part == Part::Add) this->addTheme(mColor);
        else if (part == Part::Chip && index >= 0 && index < (int)mThemes.size())
        {
            const Color& swatch = mThemes[(size_t)index];
            this->userDisplay(swatch.r, swatch.g, swatch.b, useAlpha ? swatch.a : 1.0f);
        }
    }

    void UIColorPicker::beginEdit(int which, bool resetSnapshot)
    {
        mEdit = which;
        mEditText = which == kEditHex ? this->formatHex() : this->formatChannel(which);
        mAnchor = 0;
        mCaret = mEditText.length();
        if (resetSnapshot)
        {
            mEditColor = mColor;
            mEditH = mH;
            mEditS = mS;
            mEditV = mV;
        }
        mBlink = std::chrono::steady_clock::now();
    }

    void UIColorPicker::commitEdit()
    {
        if (mEdit == kEditNone) return;
        int which = mEdit;
        String text = mEditText;
        mEdit = kEditNone;
        if (which == kEditHex) this->applyHex(text, true);
        else this->applyNumber(which, text, true);
        if (mDirty)
        {
            mDirty = false;
            this->notifyCommitted();
        }
    }

    void UIColorPicker::cancelEdit()
    {
        if (mEdit == kEditNone) return;
        mEdit = kEditNone;
        Color before = mColor;
        mColor = mEditColor;
        mH = mEditH;
        mS = mEditS;
        mV = mEditV;
        mDirty = false;
        if (!nearColor(before, mColor.r, mColor.g, mColor.b, mColor.a)) this->notifyChanged();
    }

    void UIColorPicker::finishPointer(float x, float y, bool allowClick)
    {
        Drag drag = mDrag;
        Part part = mPart;
        int index = mIndex;
        bool moved = mMoved;
        mDrag = Drag::None;
        if (drag == Drag::None) return;
        Vector2 local = this->screenToLocal(Vector2(x, y));
        if (allowClick && drag == Drag::Click && !moved)
        {
            if (part == Part::Spin) this->beginEdit(index, true);
            else if (part == Part::Hex) this->beginEdit(kEditHex, true);
            else this->activate(part, index);
        }
        else if (drag == Drag::Swatch)
        {
            if (moved && mThemeBar.contains(local.x, local.y)) this->addTheme(mDragColor);
            else if (!moved && part == Part::OldSwatch)
            {
                this->userDisplay(mOriginal.r, mOriginal.g, mOriginal.b, useAlpha ? mOriginal.a : 1.0f);
            }
        }
        if (mDirty)
        {
            mDirty = false;
            this->notifyCommitted();
        }
    }

    void UIColorPicker::stepChannel(int channel, float notches)
    {
        float step = channel == (int)Channel::Hue ? 1.0f : 0.01f;
        this->setChannel(channel, this->channelValue(channel) + notches * step);
        if (mDirty)
        {
            mDirty = false;
            this->notifyCommitted();
        }
    }

    String UIColorPicker::formatChannel(int channel) const
    {
        return String::format("%.3f", this->channelValue(channel));
    }

    String UIColorPicker::formatHex() const
    {
        float r = srgb ? mColor.r : toLinear(mColor.r);
        float g = srgb ? mColor.g : toLinear(mColor.g);
        float b = srgb ? mColor.b : toLinear(mColor.b);
        unsigned rb = (unsigned)byteOf(r);
        unsigned gb = (unsigned)byteOf(g);
        unsigned bb = (unsigned)byteOf(b);
        unsigned ab = (unsigned)byteOf(mColor.a);
        if (useAlpha) return String::format("%02X%02X%02X%02X", rb, gb, bb, ab);
        return String::format("%02X%02X%02X", rb, gb, bb);
    }

    bool UIColorPicker::applyNumber(int channel, const String& text, bool strict)
    {
        if (channel < 0 || text.isEmpty()) return false;
        if (!strict && text.at(text.length() - 1) == '.') return false;
        float value = 0.0f;
        if (!parseFloatText(text, value)) return false;
        this->setChannel(channel, value);
        return true;
    }

    bool UIColorPicker::applyHex(const String& text, bool strict)
    {
        String digits = hexDigits(text);
        size_t n = digits.length();
        bool hasAlpha = false;
        if (n == 6 || n == 8) hasAlpha = n == 8;
        else if (strict && (n == 3 || n == 4)) hasAlpha = n == 4;
        else return false;
        for (size_t i = 0; i < n; ++i)
        {
            if (nibble(digits.at(i)) < 0) return false;
        }
        float channel[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        int count = hasAlpha ? 4 : 3;
        if (n == 3 || n == 4)
        {
            for (int i = 0; i < count; ++i)
            {
                int v = nibble(digits.at((size_t)i));
                channel[i] = (float)(v * 17) / 255.0f;
            }
        }
        else
        {
            for (int i = 0; i < count; ++i)
            {
                int hi = nibble(digits.at((size_t)i * 2));
                int lo = nibble(digits.at((size_t)i * 2 + 1));
                channel[i] = (float)(hi * 16 + lo) / 255.0f;
            }
        }
        float r = channel[0];
        float g = channel[1];
        float b = channel[2];
        if (!srgb)
        {
            r = toSrgb(r);
            g = toSrgb(g);
            b = toSrgb(b);
        }
        float a = hasAlpha ? channel[3] : mColor.a;
        this->userDisplay(r, g, b, a);
        return true;
    }

    void UIColorPicker::insertEdit(const String& raw)
    {
        if (mEdit == kEditNone || raw.isEmpty()) return;
        size_t a = mAnchor < mCaret ? mAnchor : mCaret;
        size_t b = mAnchor > mCaret ? mAnchor : mCaret;
        String left = a > 0 ? mEditText.substr(0, a) : String();
        String right = b < mEditText.length() ? mEditText.substr(b, mEditText.length() - b) : String();
        String mid;
        for (size_t i = 0; i < raw.length(); ++i)
        {
            char c = raw.at(i);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
            char buf[2] = { c, '\0' };
            String next = left + mid + String(buf) + right;
            bool ok = false;
            if (mEdit == kEditHex)
            {
                String digits = hexDigits(next);
                ok = digits.length() <= 8;
                for (size_t k = 0; ok && k < digits.length(); ++k) ok = nibble(digits.at(k)) >= 0;
            }
            else
            {
                int dots = 0;
                ok = next.length() <= 16;
                for (size_t k = 0; ok && k < next.length(); ++k)
                {
                    char ch = next.at(k);
                    if (ch == '.') ++dots;
                    else if (ch < '0' || ch > '9') ok = false;
                }
                if (dots > 1) ok = false;
            }
            if (!ok) continue;
            mid = mid + String(buf);
        }
        if (mid.isEmpty() && a == b) return;
        mEditText = left + mid + right;
        mCaret = mAnchor = a + mid.length();
        if (mEdit == kEditHex) this->applyHex(mEditText, false);
        else this->applyNumber(mEdit, mEditText, false);
        mBlink = std::chrono::steady_clock::now();
    }

    void UIColorPicker::eraseSelection()
    {
        size_t a = mAnchor < mCaret ? mAnchor : mCaret;
        size_t b = mAnchor > mCaret ? mAnchor : mCaret;
        if (a == b) return;
        String left = a > 0 ? mEditText.substr(0, a) : String();
        String right = b < mEditText.length() ? mEditText.substr(b, mEditText.length() - b) : String();
        mEditText = left + right;
        mCaret = mAnchor = a;
    }

    void UIColorPicker::moveCaret(int direction, bool shift)
    {
        if (direction < 0)
        {
            if (mCaret > 0) --mCaret;
        }
        else if (mCaret < mEditText.length())
        {
            ++mCaret;
        }
        if (!shift) mAnchor = mCaret;
        mBlink = std::chrono::steady_clock::now();
    }

    size_t UIColorPicker::indexAt(float x) const
    {
        Slot slot = this->editSlot();
        if ((int)slot >= (int)Slot::Count) return 0;
        float local = x - mTextPos[(int)slot].x;
        if (local <= 0.0f) return 0;
        size_t count = mEditText.length();
        for (size_t i = 0; i < count; ++i)
        {
            float before = this->textWidth(mEditText, i);
            float after = this->textWidth(mEditText, i + 1);
            if (local < (before + after) * 0.5f) return i;
        }
        return count;
    }

    float UIColorPicker::textWidth(const String& text, size_t count) const
    {
        UIFont* font = UIFont::find(fontPath);
        if (font == nullptr || !font->isOpen() || count == 0 || text.isEmpty()) return 0.0f;
        if (count > text.length()) count = text.length();
        return font->measure(text.cstr(), count, fontSize).x;
    }

    UIColorPicker::Box UIColorPicker::editBox() const
    {
        if (mEdit == kEditHex) return mHex;
        if (mEdit >= 0 && mEdit < (int)Channel::Count) return mSpin[mEdit];
        return Box();
    }

    UIColorPicker::Slot UIColorPicker::editSlot() const
    {
        if (mEdit == kEditHex) return Slot::Hex;
        if (mEdit >= 0 && mEdit < (int)Channel::Count) return (Slot)((int)Slot::VR + mEdit);
        return Slot::Count;
    }

    void UIColorPicker::notifyChanged()
    {
        if (mSilence || !onColorChanged) return;
        mSilence = true;
        Color copy = mColor;
        onColorChanged(copy);
        mSilence = false;
    }

    void UIColorPicker::notifyCommitted()
    {
        if (mSilence || !onColorCommitted) return;
        mSilence = true;
        Color copy = mColor;
        onColorCommitted(copy);
        mSilence = false;
    }

    void UIColorPicker::mark(bool changed)
    {
        if (!changed) return;
        mDirty = true;
        this->notifyChanged();
    }

    void UIColorPicker::remember(float x, float y)
    {
        mLocal = this->screenToLocal(Vector2(x, y));
    }

    Color UIColorPicker::accentHover() const
    {
        return Color(
            accent.r + (1.0f - accent.r) * 0.18f,
            accent.g + (1.0f - accent.g) * 0.18f,
            accent.b + (1.0f - accent.b) * 0.18f,
            1.0f);
    }

    Color UIColorPicker::accentPress() const
    {
        return Color(accent.r * 0.82f, accent.g * 0.82f, accent.b * 0.82f, 1.0f);
    }
}
