#ifndef _EOKAS_UI_TEXT_H_
#define _EOKAS_UI_TEXT_H_

#include "UIWidget.h"

namespace eokas
{
    class UIFont;

    class UIText : public UIWidget
    {
    public:
        String text;
        String fontPath;
        float fontSize = 16.0f;
        UIFont* font = nullptr;
        void render(UIShape& shape) override;
    };
}

#endif//_EOKAS_UI_TEXT_H_
