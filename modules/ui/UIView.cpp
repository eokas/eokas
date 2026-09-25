#include "UIView.h"
#include "UIFont.h"

namespace eokas
{
    namespace
    {
        float minf(float a, float b)
        {
            return a < b ? a : b;
        }

        float maxf(float a, float b)
        {
            return a > b ? a : b;
        }

        float clampf(float value, float lo, float hi)
        {
            if (value < lo)
            {
                return lo;
            }
            if (value > hi)
            {
                return hi;
            }
            return value;
        }

        float thumbSpan(float inner, float minScroll, float maxScroll)
        {
            float content = (maxScroll - minScroll) + inner;
            float thumb = inner;
            if (content > 0.0f)
            {
                thumb = inner * (inner / content);
            }
            if (thumb < 12.0f)
            {
                thumb = 12.0f;
            }
            if (thumb > inner)
            {
                thumb = inner;
            }
            return thumb;
        }
    }

    UIView::UIView()
    {
        interactive = false;
        mRoot = std::make_shared<UIWidget>();
        mRoot->interactive = false;
    }

    void UIView::addChild(const std::shared_ptr<UIWidget>& child)
    {
        mRoot->children.push_back(child);
    }

    void UIView::setScroll(float x, float y)
    {
        mScrollX = x;
        mScrollY = y;
        this->updateMetrics();
    }

    bool UIView::scrollBy(float dx, float dy)
    {
        float nx = clampf(mScrollX + dx, mMinScrollX, mMaxScrollX);
        float ny = clampf(mScrollY + dy, mMinScrollY, mMaxScrollY);
        bool moved = nx != mScrollX || ny != mScrollY;
        mScrollX = nx;
        mScrollY = ny;
        if (moved)
        {
            this->placeRoot();
        }
        return moved;
    }

    Rect UIView::viewport() const
    {
        return Rect(rect.x, rect.y, mInnerW, mInnerH);
    }

    bool UIView::scrollbarContains(float localX, float localY) const
    {
        Vector2 point(localX, localY);
        if (mShowV)
        {
            Rect track = this->verticalTrack();
            if (track.contains(point))
            {
                return true;
            }
        }
        if (mShowH)
        {
            Rect track = this->horizontalTrack();
            if (track.contains(point))
            {
                return true;
            }
        }
        return false;
    }

    void UIView::placeRoot()
    {
        mRoot->rect.x = rect.x - mScrollX;
        mRoot->rect.y = rect.y - mScrollY;
    }

    float UIView::barSize() const
    {
        return scrollbarThickness > 0.0f ? scrollbarThickness : 0.0f;
    }

    void UIView::expandContent(const UIWidget* widget, float& minX, float& minY, float& maxX, float& maxY, bool& any) const
    {
        if (widget == nullptr || !widget->visible || widget->floating)
        {
            return;
        }
        float x1 = widget->rect.x + widget->rect.width;
        float y1 = widget->rect.y + widget->rect.height;
        if (!any)
        {
            minX = widget->rect.x;
            minY = widget->rect.y;
            maxX = x1;
            maxY = y1;
            any = true;
        }
        else
        {
            minX = minf(minX, widget->rect.x);
            minY = minf(minY, widget->rect.y);
            maxX = maxf(maxX, x1);
            maxY = maxf(maxY, y1);
        }
        if (dynamic_cast<const UIView*>(widget) != nullptr)
        {
            return;
        }
        for (auto& child : widget->children)
        {
            this->expandContent(child.get(), minX, minY, maxX, maxY, any);
        }
    }

    void UIView::layout(const Rect& rect)
    {
        this->rect = rect;
        for (auto& child : mRoot->children)
        {
            if (child)
            {
                child->layout(child->rect);
            }
        }
        this->updateMetrics();
    }

    void UIView::updateMetrics()
    {
        bool any = false;
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        for (auto& child : mRoot->children)
        {
            this->expandContent(child.get(), minX, minY, maxX, maxY, any);
        }

        float outerW = rect.width > 0.0f ? rect.width : 0.0f;
        float outerH = rect.height > 0.0f ? rect.height : 0.0f;
        float bar = this->barSize();
        if (!any)
        {
            mShowV = false;
            mShowH = false;
            mInnerW = outerW;
            mInnerH = outerH;
            mMinScrollX = 0.0f;
            mMaxScrollX = 0.0f;
            mMinScrollY = 0.0f;
            mMaxScrollY = 0.0f;
            mScrollX = 0.0f;
            mScrollY = 0.0f;
            mRoot->rect.width = 0.0f;
            mRoot->rect.height = 0.0f;
            this->placeRoot();
            return;
        }

        float contentW = maxX - minX;
        float contentH = maxY - minY;
        bool showV = false;
        bool showH = false;
        for (int pass = 0; pass < 2; ++pass)
        {
            float innerW = outerW - (showV ? bar : 0.0f);
            float innerH = outerH - (showH ? bar : 0.0f);
            if (innerW < 0.0f)
            {
                innerW = 0.0f;
            }
            if (innerH < 0.0f)
            {
                innerH = 0.0f;
            }
            showV = contentH > innerH + 0.5f;
            showH = contentW > innerW + 0.5f;
        }
        float innerW = outerW - (showV ? bar : 0.0f);
        float innerH = outerH - (showH ? bar : 0.0f);
        if (innerW < 0.0f)
        {
            innerW = 0.0f;
        }
        if (innerH < 0.0f)
        {
            innerH = 0.0f;
        }
        mShowV = showV;
        mShowH = showH;
        mInnerW = innerW;
        mInnerH = innerH;

        if (!showH)
        {
            mMinScrollX = minX;
            mMaxScrollX = minX;
        }
        else
        {
            mMinScrollX = minX;
            mMaxScrollX = maxX - innerW;
            if (mMaxScrollX < mMinScrollX)
            {
                mMaxScrollX = mMinScrollX;
            }
        }
        if (!showV)
        {
            mMinScrollY = minY;
            mMaxScrollY = minY;
        }
        else
        {
            mMinScrollY = minY;
            mMaxScrollY = maxY - innerH;
            if (mMaxScrollY < mMinScrollY)
            {
                mMaxScrollY = mMinScrollY;
            }
        }
        mScrollX = clampf(mScrollX, mMinScrollX, mMaxScrollX);
        mScrollY = clampf(mScrollY, mMinScrollY, mMaxScrollY);
        mRoot->rect.width = contentW;
        mRoot->rect.height = contentH;
        this->placeRoot();
    }

    Rect UIView::verticalTrack() const
    {
        return Rect(rect.x + mInnerW, rect.y, this->barSize(), mInnerH);
    }

    Rect UIView::horizontalTrack() const
    {
        return Rect(rect.x, rect.y + mInnerH, mInnerW, this->barSize());
    }

    Rect UIView::verticalThumb() const
    {
        Rect track = this->verticalTrack();
        float thumb = thumbSpan(track.height, mMinScrollY, mMaxScrollY);
        float span = mMaxScrollY - mMinScrollY;
        float travel = track.height - thumb;
        float t = (span > 0.0f) ? (mScrollY - mMinScrollY) / span : 0.0f;
        t = clampf(t, 0.0f, 1.0f);
        return Rect(track.x, track.y + t * travel, track.width, thumb);
    }

    Rect UIView::horizontalThumb() const
    {
        Rect track = this->horizontalTrack();
        float thumb = thumbSpan(track.width, mMinScrollX, mMaxScrollX);
        float span = mMaxScrollX - mMinScrollX;
        float travel = track.width - thumb;
        float t = (span > 0.0f) ? (mScrollX - mMinScrollX) / span : 0.0f;
        t = clampf(t, 0.0f, 1.0f);
        return Rect(track.x + t * travel, track.y, thumb, track.height);
    }

    void UIView::drawScrollbars(UIShape& shape) const
    {
        Rect solid = UIFont::solidUV();
        if (mShowV)
        {
            shape.addQuad(this->verticalTrack(), solid, scrollbarTrackColor);
            Color thumb = (mDrag == BarDrag::VerticalThumb) ? scrollbarPressedColor : scrollbarColor;
            shape.addQuad(this->verticalThumb(), solid, thumb);
        }
        if (mShowH)
        {
            shape.addQuad(this->horizontalTrack(), solid, scrollbarTrackColor);
            Color thumb = (mDrag == BarDrag::HorizontalThumb) ? scrollbarPressedColor : scrollbarColor;
            shape.addQuad(this->horizontalThumb(), solid, thumb);
        }
        if (mShowV && mShowH)
        {
            float bar = this->barSize();
            shape.addQuad(Rect(rect.x + mInnerW, rect.y + mInnerH, bar, bar), solid, scrollbarTrackColor);
        }
    }

    void UIView::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        if (color.a > 0.0f)
        {
            shape.addQuad(rect, UIFont::solidUV(), color);
        }

        Vector2 parent = shape.offset();
        Rect vp = this->viewport();
        Rect screenClip(parent.x + vp.x, parent.y + vp.y, vp.width, vp.height);
        Vector2 content = parent + Vector2(mRoot->rect.x, mRoot->rect.y);
        shape.pushOffset(content);
        shape.pushClip(screenClip);
        for (auto& child : mRoot->children)
        {
            if (child && !child->floating && !shape.outsideClip(child->rect))
            {
                child->render(shape);
            }
        }
        shape.popClip();
        for (auto& child : mRoot->children)
        {
            if (child && child->floating)
            {
                child->render(shape);
            }
        }
        shape.popOffset();
        this->drawScrollbars(shape);
    }

    void UIView::triggerPointerDrag(float x, float y, int button)
    {
        if (button != 0)
        {
            return;
        }
        Vector2 point(x, y);
        if (mDrag == BarDrag::None)
        {
            if (mShowV)
            {
                Rect thumb = this->verticalThumb();
                if (thumb.contains(point))
                {
                    mDrag = BarDrag::VerticalThumb;
                    mGrab = y - thumb.y;
                }
            }
            if (mDrag == BarDrag::None && mShowH)
            {
                Rect thumb = this->horizontalThumb();
                if (thumb.contains(point))
                {
                    mDrag = BarDrag::HorizontalThumb;
                    mGrab = x - thumb.x;
                }
            }
            if (mDrag == BarDrag::None && mShowV)
            {
                Rect track = this->verticalTrack();
                if (track.contains(point))
                {
                    Rect thumb = this->verticalThumb();
                    mScrollY += (y < thumb.y) ? -mInnerH : mInnerH;
                    mScrollY = clampf(mScrollY, mMinScrollY, mMaxScrollY);
                    this->placeRoot();
                    mDrag = BarDrag::Track;
                    return;
                }
            }
            if (mDrag == BarDrag::None && mShowH)
            {
                Rect track = this->horizontalTrack();
                if (track.contains(point))
                {
                    Rect thumb = this->horizontalThumb();
                    mScrollX += (x < thumb.x) ? -mInnerW : mInnerW;
                    mScrollX = clampf(mScrollX, mMinScrollX, mMaxScrollX);
                    this->placeRoot();
                    mDrag = BarDrag::Track;
                    return;
                }
            }
        }

        if (mDrag == BarDrag::VerticalThumb)
        {
            Rect track = this->verticalTrack();
            float thumb = thumbSpan(track.height, mMinScrollY, mMaxScrollY);
            float travel = track.height - thumb;
            float t = (travel > 0.0f) ? (y - mGrab - track.y) / travel : 0.0f;
            mScrollY = clampf(mMinScrollY + t * (mMaxScrollY - mMinScrollY), mMinScrollY, mMaxScrollY);
            this->placeRoot();
        }
        else if (mDrag == BarDrag::HorizontalThumb)
        {
            Rect track = this->horizontalTrack();
            float thumb = thumbSpan(track.width, mMinScrollX, mMaxScrollX);
            float travel = track.width - thumb;
            float t = (travel > 0.0f) ? (x - mGrab - track.x) / travel : 0.0f;
            mScrollX = clampf(mMinScrollX + t * (mMaxScrollX - mMinScrollX), mMinScrollX, mMaxScrollX);
            this->placeRoot();
        }
    }

    void UIView::triggerPointerRelease()
    {
        mDrag = BarDrag::None;
        UIWidget::triggerPointerRelease();
    }
}
