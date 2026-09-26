#include "UIRegion.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    UIRegion::UIRegion()
    {
        color.a = 0.0f;
        this->setHead(std::make_shared<UIWidget>());
    }

    void UIRegion::setExpanded(bool next)
    {
        if (mExpanded == next)
        {
            return;
        }
        mExpanded = next;
        if (onExpandedChanged)
        {
            onExpandedChanged(mExpanded);
        }
    }

    void UIRegion::setSpacing(float value)
    {
        if (mSpacing == value)
        {
            return;
        }
        mSpacing = value;
    }

    void UIRegion::setHead(const std::shared_ptr<UIWidget>& widget)
    {
        if (mHead == widget)
        {
            return;
        }
        mHead = widget;
        if (mHead)
        {
            mHead->onClick = [this]()
            {
                if (this->pickable)
                {
                    this->setExpanded(!this->mExpanded);
                }
            };
        }
        this->syncChildren();
    }

    void UIRegion::setBody(const std::shared_ptr<UIWidget>& widget)
    {
        if (mBody == widget)
        {
            return;
        }
        mBody = widget;
        this->syncChildren();
    }

    void UIRegion::syncChildren()
    {
        children.clear();
        if (mHead)
        {
            children.push_back(mHead);
        }
        if (mBody)
        {
            children.push_back(mBody);
        }
    }

    void UIRegion::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        float width = shape.size.x;
        if (width <= 0.0f)
        {
            width = 0.0f;
            if (mHead && mHead->shape.size.x > width)
            {
                width = mHead->shape.size.x;
            }
            if (mExpanded && mBody && mBody->shape.size.x > width)
            {
                width = mBody->shape.size.x;
            }
        }

        float headH = 0.0f;
        if (mHead && mHead->shape.size.y > 0.0f)
        {
            headH = mHead->shape.size.y;
        }

        float bodyH = 0.0f;
        if (mBody && mBody->shape.size.y > 0.0f)
        {
            bodyH = mBody->shape.size.y;
        }

        float gap = mSpacing;
        if (gap < 0.0f)
        {
            gap = 0.0f;
        }

        Vector2 headPos = Vector2::ZERO;
        if (mHead)
        {
            mHead->visible = visible;
            mHead->pickable = pickable;
            this->placeChild(*mHead, headPos, Vector2(width, headH));
            headH = mHead->shape.size.y;
        }

        bool showBody = mExpanded && visible && mBody != nullptr;
        if (mBody)
        {
            mBody->visible = showBody;
            if (showBody)
            {
                Vector2 bodyPos(0.0f, floorf(headH + gap + 0.5f));
                this->placeChild(*mBody, bodyPos, Vector2(width, bodyH));
                bodyH = mBody->shape.size.y;
            }
        }

        float height = headH;
        if (showBody)
        {
            height += gap + bodyH;
        }
        this->resize(Vector2(width, height));
        if (color.a > 0.0f)
        {
            primitive.addQuad(shape.worldTrans(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }
        UIWidget::render(primitive);
    }
}
