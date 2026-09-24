#include "UIRegion.h"
#include "UIFont.h"
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
        this->layoutParts();
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
        this->layoutParts();
    }

    void UIRegion::setHead(const std::shared_ptr<UIWidget>& widget)
    {
        if (mHead == widget)
        {
            this->layoutParts();
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
        this->layoutParts();
    }

    void UIRegion::setBody(const std::shared_ptr<UIWidget>& widget)
    {
        if (mBody == widget)
        {
            this->layoutParts();
            return;
        }
        mBody = widget;
        this->syncChildren();
        this->layoutParts();
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

    void UIRegion::layoutParts()
    {
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

        if (mHead)
        {
            mHead->visible = visible;
            mHead->interactive = interactive;
            mHead->rect.x = floorf(rect.x + 0.5f);
            mHead->rect.y = floorf(rect.y + 0.5f);
            mHead->rect.width = width;
        }

        bool showBody = mExpanded && visible && mBody != nullptr;
        if (mBody)
        {
            mBody->visible = showBody;
            if (showBody)
            {
                float y = rect.y + headH + gap;
                mBody->rect.x = floorf(rect.x + 0.5f);
                mBody->rect.y = floorf(y + 0.5f);
                mBody->rect.width = width;
            }
        }

        float height = headH;
        if (showBody)
        {
            height += gap + bodyH;
        }
        rect.width = width;
        rect.height = height;
    }

    void UIRegion::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        this->layoutParts();
        if (color.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), color);
        }
        UIWidget::render(shape);
    }
}
