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
        return UIWidget::contains(point);
    }

    void UIRectangle::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Matrix3 localToScreen = matrixLocalToScreen();
        if (shape.size.x > 0.0f && shape.size.y > 0.0f)
        {
            primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), this->activeFill());
            std::vector<Vector2> loop;
            loop.push_back(Vector2(0.0f, 0.0f));
            loop.push_back(Vector2(shape.size.x, 0.0f));
            loop.push_back(shape.size);
            loop.push_back(Vector2(0.0f, shape.size.y));
            if (border.thickness > 0.0f)
            {
                UIStroke::path(primitive, localToScreen, loop, true, border);
            }
            if (selected && stroke.thickness > 0.0f)
            {
                float pad = (border.thickness + stroke.thickness) * 0.5f;
                std::vector<Vector2> outer;
                outer.push_back(Vector2(-pad, -pad));
                outer.push_back(Vector2(shape.size.x + pad, -pad));
                outer.push_back(Vector2(shape.size.x + pad, shape.size.y + pad));
                outer.push_back(Vector2(-pad, shape.size.y + pad));
                UIStroke::path(primitive, localToScreen, outer, true, stroke);
            }
        }
        UIWidget::render(primitive);
    }
}
