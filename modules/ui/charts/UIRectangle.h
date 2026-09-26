#ifndef _EOKAS_UI_RECTANGLE_H_
#define _EOKAS_UI_RECTANGLE_H_

#include "../widgets/UIChart.h"

namespace eokas
{
    class UIRectangle : public UIChart
    {
    public:
        UIStrokeStyle border { Color(0.95f, 0.97f, 1.0f, 1.0f) };

        UIRectangle();
        bool contains(const Vector2& point) const override;
        void render(UIPrimitive& primitive) override;
    };
}

#endif//_EOKAS_UI_RECTANGLE_H_
