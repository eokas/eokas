#ifndef _EOKAS_UI_PALETTE_H_
#define _EOKAS_UI_PALETTE_H_

#include "../UIWidget.h"
#include "UIGrid.h"

namespace eokas
{
    class UIPalette : public UIWidget
    {
    public:
        static constexpr int kColumns = 8;
        static constexpr int kRows = 5;
        static constexpr int kCount = 40;

        float uiScale = 1.0f;
        float cellSize = 20.0f;
        float padding = 4.0f;
        float spacing = 2.0f;
        Color background { 0.145f, 0.152f, 0.176f, 1.0f };
        Color line { 0.040f, 0.043f, 0.055f, 1.0f };
        Color accent { 0.345f, 0.545f, 0.930f, 1.0f };
        std::function<void(const Color&)> onColorChanged;

        UIPalette();

        static Vector2 preferredSize(float scale, float padding = 4.0f, float spacing = 2.0f, float cellSize = 20.0f);
        static Color paletteColor(int index);

        const Color& color() const { return mColor; }
        int selectedIndex() const { return mIndex; }
        void setColor(const Color& value);
        void setIndex(int index);

        void layout() override;
        void render(UIPrimitive& primitive) override;
        bool handleClick(float screenX, float screenY) override;

    private:
        std::shared_ptr<UIGrid> mGrid;
        Color mColor { 0.0f, 0.0f, 0.0f, 0.0f };
        int mIndex = 0;
        bool mSilence = false;

        float scaleOf() const;
        int hitIndex(float localX, float localY) const;
        Rect cellRect(int index) const;
        void notifyChanged();
    };
}

#endif//_EOKAS_UI_PALETTE_H_
