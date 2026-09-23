#ifndef _EOKAS_UI_TEXT_H_
#define _EOKAS_UI_TEXT_H_

#include "UIWidget.h"

namespace eokas
{
    class UIFont;

    class UIText : public UIWidget
    {
    public:
        static constexpr float kDefaultFontSize = 16.0f;

        UIText() { interactive = false; }
        String text;
        String fontPath;
        float fontSize = kDefaultFontSize;
        UIFont* font = nullptr;
        void render(UIShape& shape) override;
    };
}

#endif//_EOKAS_UI_TEXT_H_
