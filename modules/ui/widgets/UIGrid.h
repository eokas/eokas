#ifndef _EOKAS_UI_GRID_H_
#define _EOKAS_UI_GRID_H_

#include "../UIWidget.h"

namespace eokas
{
    class UIGrid : public UIWidget
    {
    public:
        UIGrid() { interactive = false; }
        float padding = 8.0f;
        float spacing = 8.0f;
        int columns = 1;
        float cellWidth = 0.0f;
        float cellHeight = 0.0f;
        float rowSpacing = -1.0f;
        void addChild(const std::shared_ptr<UIWidget>& child);
        void layout(const Rect& rect) override;
        void refit();
        void render(UIPrimitive& primitive) override;
    };
}

#endif//_EOKAS_UI_GRID_H_
