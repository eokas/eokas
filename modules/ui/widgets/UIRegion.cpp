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
        float width = rect.width;
        if (width <= 0.0f)
        {
            width = 0.0f;
            if (mHead && mHead->rect.width > width)
            {
                width = mHead->rect.width;
            }
            if (mExpanded && mBody && mBody->rect.width > width)
            {
                width = mBody->rect.width;
            }
        }

        float headH = 0.0f;
        if (mHead && mHead->rect.height > 0.0f)
        {
            headH = mHead->rect.height;
        }

        float bodyH = 0.0f;
        if (mBody && mBody->rect.height > 0.0f)
        {
            bodyH = mBody->rect.height;
        }

        float gap = mSpacing;
        if (gap < 0.0f)
        {
            gap = 0.0f;
        }

        float x = floorf(rect.x + 0.5f);
        float y = floorf(rect.y + 0.5f);
        if (mHead)
        {
            mHead->visible = visible;
            mHead->interactive = interactive;
            mHead->layout(Rect(x, y, width, headH));
            headH = mHead->rect.height;
        }

        bool showBody = mExpanded && visible && mBody != nullptr;
        if (mBody)
        {
            mBody->visible = showBody;
            if (showBody)
            {
                float bodyY = floorf(rect.y + headH + gap + 0.5f);
                mBody->layout(Rect(x, bodyY, width, bodyH));
                bodyH = mBody->rect.height;
            }
        }

        float height = headH;
        if (showBody)
        {
            height += gap + bodyH;
        }
        this->rect.width = width;
        this->rect.height = height;
    }

    void UIRegion::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        if (color.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), color);
        }
        UIWidget::render(shape);
    }
}
