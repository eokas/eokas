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
            interactive = false;
            style.fontSize = kDefaultFontSize;
            fill.a = 0.0f;
        }
        String text;
        UITextStyle style;
        void render(UIPrimitive& primitive) override;
    };
}

#endif//_EOKAS_UI_TEXT_H_
