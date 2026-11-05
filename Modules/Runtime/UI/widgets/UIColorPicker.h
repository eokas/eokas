#ifndef _EOKAS_UI_COLOR_PICKER_H_
#define _EOKAS_UI_COLOR_PICKER_H_

#include "UIText.h"
#include <vector>

namespace eokas
{
    enum class UIColorPickerMode
    {
        Wheel,
        Spectrum
    };

    // Editor-style color picker: wheel or spectrum, saturation/value bars,
    // new/old swatches, hex, RGBA + HSV fields, themes, sRGB, and an eyedropper.
    // color() is the displayed color. The sRGB toggle switches RGB and hex
    // between that display value and its linear equivalent.
    class UIColorPicker : public UIWidget
    {
    public:
        UIColorPickerMode mode = UIColorPickerMode::Wheel;
        bool useAlpha = true;
        bool srgb = true;
        float uiScale = 1.0f;
        String fontPath;
        float fontSize = 13.0f;

        Color background { 0.145f, 0.152f, 0.176f, 1.0f };
        Color field { 0.067f, 0.071f, 0.086f, 1.0f };
        Color ink { 0.925f, 0.933f, 0.953f, 1.0f };
        Color muted { 0.620f, 0.650f, 0.700f, 1.0f };
        Color accent { 0.345f, 0.545f, 0.930f, 1.0f };
        Color button { 0.227f, 0.247f, 0.306f, 1.0f };
        Color buttonHover { 0.290f, 0.325f, 0.400f, 1.0f };
        Color buttonPress { 0.165f, 0.180f, 0.227f, 1.0f };
        Color line { 0.040f, 0.043f, 0.055f, 1.0f };

        std::function<void(const Color&)> onColorChanged;
        std::function<void(const Color&)> onColorCommitted;
        std::function<void()> onCancelled;
        // Screen-space sample used by the eyedropper. Return a negative alpha
        // when the pixel cannot be read. Alpha on the returned color is ignored.
        std::function<Color(float x, float y)> sampleScreen;
        std::function<void(const String&)> writeClipboard;
        std::function<String()> readClipboard;

        UIColorPicker();

        static Vector2 preferredSize(float scale);

        const Color& color() const { return mColor; }
        const Color& originalColor() const { return mOriginal; }
        void setColor(const Color& value);
        void setColor(const Color& value, bool resetOriginal);
        void setFont(const String& path, float size);

        bool eyeDropping() const { return mEye; }
        void sampleEyeDropper(float x, float y, bool commit);
        void cancelEyeDropper();

        void layout() override;
        void render(UIPrimitive& primitive) override;
        bool handlePointerEnter(const UIPointerEvent& event) override;
        bool handlePointerLeave(const UIPointerEvent& event) override;
        bool handlePointerMove(const UIPointerEvent& event) override;
        bool handlePointerPress(const UIPointerEvent& event) override;
        bool handlePointerRelease(const UIPointerEvent& event) override;
        bool handleWheel(float screenX, float screenY, float deltaX, float deltaY) override;
        bool handleKeyPress(const UIKeyEvent& event) override;
        bool handleKeyChar(uint32_t character) override;
        void handleFocusLoss() override;

    private:
        enum class Slot : int
        {
            Themes, Srgb, NewCap, OldCap,
            Wheel, Spectrum, Pick, Ok, Cancel, Hex,
            NR, NG, NB, NA, NH, NS, NV,
            VR, VG, VB, VA, VH, VS, VV,
            Count
        };

        enum class Channel : int
        {
            Red = 0, Green, Blue, Alpha, Hue, Saturation, Value, Count
        };

        enum class Part : int
        {
            None, Add, Srgb, Wheel, Bar0, Bar1,
            NewSwatch, OldSwatch, Hex,
            ModeWheel, ModeSpectrum, Dropper, Ok, Cancel,
            Track, Spin, Chip
        };

        enum class Drag : int
        {
            None, Wheel, Bar0, Bar1, Track, Scrub, Text, Swatch, Click
        };

        enum class Axis : int
        {
            None, Saturation, Value, Hue, Alpha
        };

        struct Box
        {
            float x = 0.0f;
            float y = 0.0f;
            float w = 0.0f;
            float h = 0.0f;

            bool contains(float px, float py) const
            {
                return w > 0.0f && h > 0.0f && px >= x && py >= y && px < x + w && py < y + h;
            }

            Rect toRect() const { return Rect(x, y, w, h); }
        };

        struct Chip
        {
            Box box;
            int index = -1;
        };

        struct Hit
        {
            Part part = Part::None;
            int index = -1;
        };

        static constexpr int kEditNone = -1;
        static constexpr int kEditHex = -2;

        std::shared_ptr<UIText> mText[(int)Slot::Count];
        Vector2 mTextPos[(int)Slot::Count];
        std::vector<Color> mThemes;
        std::vector<Chip> mChips;
        int mBuiltin = 0;

        Box mThemeBar;
        Box mAdd;
        Box mSrgb;
        Box mCheck;
        Box mWheel;
        Box mBar0;
        Box mBar1;
        Box mSquare;
        Box mNewCap;
        Box mNewSwatch;
        Box mOldCap;
        Box mOldSwatch;
        Box mHex;
        Box mModeWheel;
        Box mModeSpectrum;
        Box mDropper;
        Box mRow[7];
        Box mTrack[7];
        Box mSpin[7];
        Box mOk;
        Box mCancel;
        float mCenterX = 0.0f;
        float mCenterY = 0.0f;
        float mOuter = 0.0f;
        float mInner = 0.0f;
        float mScale = 1.0f;

        Color mColor { 1.0f, 1.0f, 1.0f, 1.0f };
        Color mOriginal { 1.0f, 1.0f, 1.0f, 1.0f };
        float mH = 0.0f;
        float mS = 0.0f;
        float mV = 1.0f;

        Drag mDrag = Drag::None;
        Part mPart = Part::None;
        int mIndex = -1;
        bool mMoved = false;
        bool mWheelSV = false;
        bool mInside = false;
        bool mDirty = false;
        bool mSilence = false;
        bool mEye = false;
        Vector2 mLocal { 0.0f, 0.0f };
        float mPressX = 0.0f;
        float mPressY = 0.0f;
        float mPressNumber = 0.0f;
        size_t mPressCaret = 0;
        Color mDragColor { 1.0f, 1.0f, 1.0f, 1.0f };
        Color mEyeColor { 1.0f, 1.0f, 1.0f, 1.0f };
        float mEyeH = 0.0f;
        float mEyeS = 0.0f;
        float mEyeV = 1.0f;

        int mEdit = kEditNone;
        String mEditText;
        size_t mCaret = 0;
        size_t mAnchor = 0;
        Color mEditColor { 1.0f, 1.0f, 1.0f, 1.0f };
        float mEditH = 0.0f;
        float mEditS = 0.0f;
        float mEditV = 1.0f;
        std::chrono::steady_clock::time_point mBlink;

        float scaleOf() const;
        void applyFont();
        void syncTexts();
        void placeText(Slot slot, const Box& box, float align);
        Hit hitTest(float x, float y) const;
        bool sameField(const Hit& hit) const;
        int channelOf(const Hit& hit) const;
        Axis barAxis(int index) const;

        float channelValue(int channel) const;
        void setChannel(int channel, float value);
        void writeDisplay(float r, float g, float b, float a);
        void writeHsv(float h, float s, float v, float a);
        void userDisplay(float r, float g, float b, float a);
        void userHsv(float h, float s, float v, float a);

        void applyGraphic(float x, float y);
        void applyBar(int index, float x, float y);
        void applyTrack(int channel, float x, float y);
        void scrub(float x);
        void addTheme(const Color& value);
        void activate(Part part, int index);
        void beginEdit(int which, bool resetSnapshot);
        void commitEdit();
        void cancelEdit();
        void finishPointer(float x, float y, bool allowClick);
        void stepChannel(int channel, float notches);

        String formatChannel(int channel) const;
        String formatHex() const;
        bool applyNumber(int channel, const String& text, bool strict);
        bool applyHex(const String& text, bool strict);
        void insertEdit(const String& raw);
        void eraseSelection();
        void moveCaret(int direction, bool shift);
        size_t indexAt(float x) const;
        float textWidth(const String& text, size_t count) const;
        Box editBox() const;
        Slot editSlot() const;

        void notifyChanged();
        void notifyCommitted();
        void mark(bool changed);

        void remember(float x, float y);
        Color accentHover() const;
        Color accentPress() const;
    };
}

#endif//_EOKAS_UI_COLOR_PICKER_H_
