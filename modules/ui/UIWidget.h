#ifndef _EOKAS_UI_WIDGET_H_
#define _EOKAS_UI_WIDGET_H_

#include "UIShape.h"
#include <memory>
#include <vector>

namespace eokas
{
    class UIWidget
    {
    public:
        Rect rect;
        Color color { 1.0f, 1.0f, 1.0f, 1.0f };
        bool visible = true;
        std::vector<std::shared_ptr<UIWidget>> children;

        virtual ~UIWidget() = default;
        virtual void render(UIShape& shape);
    };
}

#endif//_EOKAS_UI_WIDGET_H_
