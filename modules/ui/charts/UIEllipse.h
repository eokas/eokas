#ifndef _EOKAS_UI_ELLIPSE_H_
#define _EOKAS_UI_ELLIPSE_H_

#include "../widgets/UIChart.h"

namespace eokas
{
    class UIEllipse : public UIChart
    {
    public:
        int segments = 48;

        UIEllipse();
        bool contains(const Vector2& point) const override;
        void render(UIPrimitive& primitive) override;
    };
}

#endif//_EOKAS_UI_ELLIPSE_H_
