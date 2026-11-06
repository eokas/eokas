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
        UIList() { pickable = false; color = Color(1.0f, 1.0f, 1.0f, 1.0f); }
        UIDirection direction = UIDirection::Vertical;
        float padding = 8.0f;
        float spacing = 8.0f;
        bool centerCross = false;
        void refit();
        void layout() override;
        void render(UIPrimitive& primitive) override;
    };
}

#endif//_EOKAS_UI_LIST_H_
