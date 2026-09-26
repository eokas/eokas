#include "UIRegion.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    UIRegion::UIRegion()
    {
        fill.a = 0.0f;
        this->setHead(std::make_shared<UIWidget>());
    }

    void UIRegion::setExpanded(bool next)
    {
        if (mExpanded == next)
        {
            return;
        }
        mExpanded = next;
        this->layout(this->rect);
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
        this->layout(this->rect);
    }

    void UIRegion::setHead(const std::shared_ptr<UIWidget>& widget)
    {
        if (mHead == widget)
        {
            this->layout(this->rect);
            return;
        }
        mHead = widget;
        if (mHead)
        {
            mHead->onClick = [this]()
            {
                if (this->interactive)
                {
                    this->setExpanded(!this->mExpanded);
                }
            };
        }
        this->syncChildren();
        this->layout(this->rect);
    }

    void UIRegion::setBody(const std::shared_ptr<UIWidget>& widget)
    {
        if (mBody == widget)
        {
            this->layout(this->rect);
            return;
        }
        mBody = widget;
        this->syncChildren();
        this->layout(this->rect);
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

    void UIRegion::layout(const Rect& rect)
    {
        this->rect = rect;
        float width = rect.size.x;
        if (width <= 0.0f)
        {
            width = 0.0f;
            if (mHead && mHead->rect.size.x > width)
            {
                width = mHead->rect.size.x;
            }
            if (mExpanded && mBody && mBody->rect.size.x > width)
            {
                width = mBody->rect.size.x;
            }
        }

        float headH = 0.0f;
        if (mHead && mHead->rect.size.y > 0.0f)
        {
            headH = mHead->rect.size.y;
        }

        float bodyH = 0.0f;
        if (mBody && mBody->rect.size.y > 0.0f)
        {
            bodyH = mBody->rect.size.y;
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
            mHead->interactive = interactive;
            mHead->layout(Rect(headPos, Vector2(width, headH)));
            headH = mHead->rect.size.y;
        }

        bool showBody = mExpanded && visible && mBody != nullptr;
        if (mBody)
        {
            mBody->visible = showBody;
            if (showBody)
            {
                Vector2 bodyPos(0.0f, floorf(headH + gap + 0.5f));
                mBody->layout(Rect(bodyPos, Vector2(width, bodyH)));
                bodyH = mBody->rect.size.y;
            }
        }

        float height = headH;
        if (showBody)
        {
            height += gap + bodyH;
        }
        this->rect.size.x = width;
        this->rect.size.y = height;
    }

    void UIRegion::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        if (fill.a > 0.0f)
        {
            primitive.addQuad(rect, UIFont::solidUV(), fill);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
