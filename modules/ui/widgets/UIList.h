#ifndef _EOKAS_UI_LIST_H_
#define _EOKAS_UI_LIST_H_

#include "../UIWidget.h"

namespace eokas
{
    enum class UIDirection
    {
        Horizontal,
        Vertical
    };

    class UIList : public UIWidget
    {
    public:
        UIList() { pickable = false; }
        UIDirection direction = UIDirection::Vertical;
        float padding = 8.0f;
        float spacing = 8.0f;
        void addChild(const std::shared_ptr<UIWidget>& child);
        void layout(const Rect& rect) override;
        void refit();
        void render(UIPrimitive& primitive) override;
    };
}

#endif//_EOKAS_UI_LIST_H_
