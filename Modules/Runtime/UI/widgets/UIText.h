#ifndef _EOKAS_UI_TEXT_H_
#define _EOKAS_UI_TEXT_H_

#include "../UIWidget.h"

namespace eokas
{
    class UIFont;

    class UIText : public UIWidget
    {
    public:
        static constexpr float kDefaultFontSize = 16.0f;

        UIText()
        {
            pickable = false;
            style.fontSize = kDefaultFontSize;
            color.a = 0.0f;
        }
        String text;
        UITextStyle style;
        float maxWidth = 0.0f;
        void layout() override;
        void render(UIPrimitive& primitive) override;
    private:
        String mShown;
    };
}

#endif//_EOKAS_UI_TEXT_H_
