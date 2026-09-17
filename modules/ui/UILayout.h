#ifndef _EOKAS_UI_LAYOUT_H_
#define _EOKAS_UI_LAYOUT_H_

#include "UIWidget.h"

namespace eokas
{
    enum class UILayoutDirection
    {
        Horizontal,
        Vertical
    };

    class UILayout : public UIWidget
    {
    public:
        UILayoutDirection direction = UILayoutDirection::Vertical;
        float padding = 8.0f;
        float spacing = 8.0f;
        void addChild(const std::shared_ptr<UIWidget>& child);
        void layout();
        void render(UIShape& shape) override;
    };
}

#endif//_EOKAS_UI_LAYOUT_H_
