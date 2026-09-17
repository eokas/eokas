#ifndef _EOKAS_UI_IMAGE_H_
#define _EOKAS_UI_IMAGE_H_

#include "UIWidget.h"

namespace eokas
{
    class UIImage : public UIWidget
    {
    public:
        Rect uv { 0.0f, 0.0f, 1.0f, 1.0f };
        void render(UIShape& shape) override;
    };
}

#endif//_EOKAS_UI_IMAGE_H_
