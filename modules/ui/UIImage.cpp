#include "UIImage.h"

namespace eokas
{
    void UIImage::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        shape.addQuad(rect, uv, color);
        UIWidget::render(shape);
    }
}
