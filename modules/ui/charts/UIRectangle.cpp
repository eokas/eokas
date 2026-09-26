#include "UIRectangle.h"
#include "../UIFont.h"
#include "../UIStroke.h"

#include <vector>

namespace eokas
{
    UIRectangle::UIRectangle()
    {
        border.thickness = 2.0f;
    }

    bool UIRectangle::contains(const Vector2& point) const
    {
        Vector2 end = rect.origin + rect.size;
        return point.x >= rect.origin.x && point.x <= end.x && point.y >= rect.origin.y && point.y <= end.y;
    }

    void UIRectangle::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        if (rect.size.x > 0.0f && rect.size.y > 0.0f)
        {
            primitive.addQuad(rect, UIFont::solidUV(), this->activeFill());
            std::vector<Vector2> loop;
            loop.push_back(rect.origin);
            loop.push_back(rect.origin + Vector2(rect.size.x, 0.0f));
            loop.push_back(rect.origin + rect.size);
            loop.push_back(rect.origin + Vector2(0.0f, rect.size.y));
            if (border.thickness > 0.0f)
            {
                UIStroke::path(primitive, loop, true, border);
            }
            if (selected && stroke.thickness > 0.0f)
            {
                float pad = (border.thickness + stroke.thickness) * 0.5f;
                std::vector<Vector2> outer;
                outer.push_back(rect.origin + Vector2(-pad, -pad));
                outer.push_back(rect.origin + Vector2(rect.size.x + pad, -pad));
                outer.push_back(rect.origin + Vector2(rect.size.x + pad, rect.size.y + pad));
                outer.push_back(rect.origin + Vector2(-pad, rect.size.y + pad));
                UIStroke::path(primitive, outer, true, stroke);
            }
        }
        primitive.popOrigin();
        this->placeLabel();
        UIWidget::render(primitive);
    }
}
