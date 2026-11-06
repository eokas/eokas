#ifndef _EOKAS_UI_TAG_H_
#define _EOKAS_UI_TAG_H_

#include "UIText.h"

namespace eokas
{
    class UITag : public UIWidget
    {
    public:
        float paddingX = 6.0f;
        float paddingY = 3.0f;
        float closeGap = 4.0f;
        Color background { Color(0.39f, 0.40f, 0.95f, 1.0f) };
        std::function<void()> onClose;

        UITag();
        UIText& caption();
        void layout() override;
        void render(UIPrimitive& primitive) override;
        bool handleClick(float screenX, float screenY) override;

    private:
        float closeHit() const;
        float rightPad() const;
        Rect closeRect() const;
        std::shared_ptr<UIText> mCaption;
    };
}

#endif
