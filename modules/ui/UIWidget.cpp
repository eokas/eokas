#include "UIWidget.h"

namespace eokas
{
    void UIWidget::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        for (auto& child : children)
        {
            if (child)
            {
                child->render(shape);
            }
        }
    }
}
