#include "UITag.h"
#include "../UIFont.h"
#include "../UIStroke.h"
#include <cmath>

namespace eokas
{
    UITag::UITag()
    {
        mCaption = std::make_shared<UIText>();
        this->attachChild(mCaption);
    }

    UIText& UITag::caption()
    {
        return *mCaption;
    }

    float UITag::closeHit() const
    {
        float hit = mCaption->shape.size.y;
        if (hit < 14.0f)
        {
            hit = 14.0f;
        }
        return hit;
    }

    float UITag::rightPad() const
    {
        if (!onClose)
        {
            return paddingX;
        }
        return closeGap + this->closeHit() + paddingX;
    }

    Rect UITag::closeRect() const
    {
        float hit = this->closeHit();
        if (hit > shape.size.y && shape.size.y > 0.0f)
        {
            hit = shape.size.y;
        }
        float x = shape.size.x - paddingX - hit;
        float y = (shape.size.y - hit) * 0.5f;
        if (y < 0.0f)
        {
            y = 0.0f;
        }
        return Rect(x, y, hit, hit);
    }

    void UITag::layout()
    {
        if (!visible)
        {
            return;
        }
        mCaption->layout();
        float width = paddingX + mCaption->shape.size.x + this->rightPad();
        float height = paddingY * 2.0f + mCaption->shape.size.y;
        if (onClose)
        {
            float hit = this->closeHit();
            if (height < hit)
            {
                height = hit;
            }
        }
        Vector2 topLeft = shape.origin - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
        shape.setBox(topLeft, Vector2(floorf(width + 0.5f), floorf(height + 0.5f)));
        float captionY = (height - mCaption->shape.size.y) * 0.5f;
        if (captionY < 0.0f)
        {
            captionY = 0.0f;
        }
        this->placeChild(*mCaption, Vector2(floorf(paddingX + 0.5f), floorf(captionY + 0.5f)));
    }

    void UITag::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.addQuad(matrixLocalToScreen(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), background);
        UIWidget::render(primitive);
        if (!onClose || !this->isHovered())
        {
            return;
        }
        Rect box = this->closeRect();
        float inset = box.size.x * 0.28f;
        if (inset < 3.0f)
        {
            inset = 3.0f;
        }
        if (inset * 2.0f >= box.size.x)
        {
            inset = box.size.x * 0.25f;
        }
        Vector2 tl(box.origin.x + inset, box.origin.y + inset);
        Vector2 br(box.origin.x + box.size.x - inset, box.origin.y + box.size.y - inset);
        Vector2 tr(box.origin.x + box.size.x - inset, box.origin.y + inset);
        Vector2 bl(box.origin.x + inset, box.origin.y + box.size.y - inset);
        UIStrokeStyle style;
        style.thickness = 1.5f;
        style.color = mCaption->style.color;
        Matrix3 localToScreen = matrixLocalToScreen();
        UIStroke::segment(primitive, localToScreen, tl, br, style);
        UIStroke::segment(primitive, localToScreen, tr, bl, style);
    }

    bool UITag::handleClick(float screenX, float screenY)
    {
        Vector2 local = this->screenToLocal(Vector2(screenX, screenY));
        if (onClose)
        {
            float left = shape.size.x - this->rightPad();
            if (local.x >= left && local.x <= shape.size.x && local.y >= 0.0f && local.y <= shape.size.y)
            {
                onClose();
                return true;
            }
        }
        return UIWidget::handleClick(screenX, screenY);
    }
}
