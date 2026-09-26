#include "UIView.h"
#include "../UIFont.h"

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
        pickable = false;
        mRoot = std::make_shared<UIWidget>();
        mRoot->pickable = false;
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
        return Rect(shape.left(), shape.top(), mInnerW, mInnerH);
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

    UIWidget* UIView::pick(const Vector2& point)
    {
        if (!visible || shape.scale.x == 0.0f || shape.scale.y == 0.0f)
        {
            return nullptr;
        }
        Vector2 local = shape.toLocal(point);
        Vector2 layout = Vector2(shape.left(), shape.top()) + local;
        bool inside = this->contains(layout);
        bool contentScale = mRoot && mRoot->shape.scale.x != 0.0f && mRoot->shape.scale.y != 0.0f;
        Vector2 content = Vector2::ZERO;
        if (contentScale)
        {
            content = mRoot->shape.toLocal(local);
            for (auto it = mRoot->children.rbegin(); it != mRoot->children.rend(); ++it)
            {
                if (*it && (*it)->floating)
                {
                    if (UIWidget* hit = (*it)->pick(content))
                    {
                        return hit;
                    }
                }
            }
        }
        if (this->scrollbarContains(layout.x, layout.y))
        {
            return this;
        }
        if (!this->viewport().contains(layout))
        {
            if (pickable && inside)
            {
                return this;
            }
            return nullptr;
        }
        if (contentScale)
        {
            for (auto it = mRoot->children.rbegin(); it != mRoot->children.rend(); ++it)
            {
                if (*it && !(*it)->floating)
                {
                    if (UIWidget* hit = (*it)->pick(content))
                    {
                        return hit;
                    }
                }
            }
        }
        if (pickable && inside)
        {
            return this;
        }
        return nullptr;
    }

    void UIView::placeRoot()
    {
        mRoot->shape.origin = (Vector2(-mScrollX, -mScrollY)) + mRoot->shape.pivot * mRoot->shape.size;
    }

    float UIView::barSize() const
    {
        return scrollbarThickness > 0.0f ? scrollbarThickness : 0.0f;
    }

    void UIView::expandContent(const UIWidget* widget, const Vector2& origin, const Vector2& scale, float& minX, float& minY, float& maxX, float& maxY, bool& any) const
    {
        if (widget == nullptr || !widget->visible || widget->floating)
        {
            return;
        }
        Vector2 topLeft = origin + scale * widget->shape.toParent(Vector2(widget->shape.left(), widget->shape.top()));
        Vector2 end = topLeft + scale * widget->shape.scale * widget->shape.size;
        float x0 = topLeft.x < end.x ? topLeft.x : end.x;
        float y0 = topLeft.y < end.y ? topLeft.y : end.y;
        float x1 = topLeft.x > end.x ? topLeft.x : end.x;
        float y1 = topLeft.y > end.y ? topLeft.y : end.y;
        if (!any)
        {
            minX = x0;
            minY = y0;
            maxX = x1;
            maxY = y1;
            any = true;
        }
        else
        {
            minX = minf(minX, x0);
            minY = minf(minY, y0);
            maxX = maxf(maxX, x1);
            maxY = maxf(maxY, y1);
        }
        if (dynamic_cast<const UIView*>(widget) != nullptr)
        {
            return;
        }
        Vector2 childScale = scale * widget->shape.scale;
        for (auto& child : widget->children)
        {
            this->expandContent(child.get(), topLeft, childScale, minX, minY, maxX, maxY, any);
        }
    }

    void UIView::layout(const Rect& rect)
    {
        {
            Rect _box = rect;
            shape.size = _box.size;
            shape.origin = _box.origin + shape.pivot * shape.size;
        }
        for (auto& child : mRoot->children)
        {
            if (child)
            {
                child->layout(Rect(child->shape.left(), child->shape.top(), child->shape.size.x, child->shape.size.y));
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
            this->expandContent(child.get(), Vector2::ZERO, mRoot->shape.scale, minX, minY, maxX, maxY, any);
        }

        float outerW = shape.size.x > 0.0f ? shape.size.x : 0.0f;
        float outerH = shape.size.y > 0.0f ? shape.size.y : 0.0f;
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
            mRoot->shape.origin.x += mRoot->shape.pivot.x * ((0.0f) - mRoot->shape.size.x);
            mRoot->shape.size.x = 0.0f;
            mRoot->shape.origin.y += mRoot->shape.pivot.y * ((0.0f) - mRoot->shape.size.y);
            mRoot->shape.size.y = 0.0f;
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
        mRoot->shape.origin.x += mRoot->shape.pivot.x * ((contentW) - mRoot->shape.size.x);
        mRoot->shape.size.x = contentW;
        mRoot->shape.origin.y += mRoot->shape.pivot.y * ((contentH) - mRoot->shape.size.y);
        mRoot->shape.size.y = contentH;
        this->placeRoot();
    }

    Rect UIView::verticalTrack() const
    {
        return Rect(shape.left() + mInnerW, shape.top(), this->barSize(), mInnerH);
    }

    Rect UIView::horizontalTrack() const
    {
        return Rect(shape.left(), shape.top() + mInnerH, mInnerW, this->barSize());
    }

    Rect UIView::verticalThumb() const
    {
        Rect track = this->verticalTrack();
        float thumb = thumbSpan(track.size.y, mMinScrollY, mMaxScrollY);
        float span = mMaxScrollY - mMinScrollY;
        float travel = track.size.y - thumb;
        float t = (span > 0.0f) ? (mScrollY - mMinScrollY) / span : 0.0f;
        t = clampf(t, 0.0f, 1.0f);
        return Rect(track.origin.x, track.origin.y + t * travel, track.size.x, thumb);
    }

    Rect UIView::horizontalThumb() const
    {
        Rect track = this->horizontalTrack();
        float thumb = thumbSpan(track.size.x, mMinScrollX, mMaxScrollX);
        float span = mMaxScrollX - mMinScrollX;
        float travel = track.size.x - thumb;
        float t = (span > 0.0f) ? (mScrollX - mMinScrollX) / span : 0.0f;
        t = clampf(t, 0.0f, 1.0f);
        return Rect(track.origin.x + t * travel, track.origin.y, thumb, track.size.y);
    }

    void UIView::drawScrollbars(UIPrimitive& primitive) const
    {
        Rect solid = UIFont::solidUV();
        if (mShowV)
        {
            primitive.addQuad(this->verticalTrack(), solid, scrollbarTrack);
            Color thumb = (mDrag == BarDrag::VerticalThumb) ? scrollbarPressed : scrollbar;
            primitive.addQuad(this->verticalThumb(), solid, thumb);
        }
        if (mShowH)
        {
            primitive.addQuad(this->horizontalTrack(), solid, scrollbarTrack);
            Color thumb = (mDrag == BarDrag::HorizontalThumb) ? scrollbarPressed : scrollbar;
            primitive.addQuad(this->horizontalThumb(), solid, thumb);
        }
        if (mShowV && mShowH)
        {
            float bar = this->barSize();
            primitive.addQuad(Rect(shape.left() + mInnerW, shape.top() + mInnerH, bar, bar), solid, scrollbarTrack);
        }
    }

    void UIView::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(shape.origin, shape.scale);
        if (color.a > 0.0f)
        {
            primitive.addQuad(Rect(shape.left(), shape.top(), shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }

        Rect vp = this->viewport();
        primitive.pushClip(primitive.toScreen(vp));
        Vector2 contentScale = primitive.scale() * mRoot->shape.scale;
        primitive.pushTransform(primitive.toScreen(Vector2(shape.left(), shape.top()) + Vector2(mRoot->shape.left(), mRoot->shape.top())), contentScale);
        for (auto& child : mRoot->children)
        {
            if (child && !child->floating && !primitive.outsideClip(child->visualRect()))
            {
                child->render(primitive);
            }
        }
        primitive.popClip();
        for (auto& child : mRoot->children)
        {
            if (child && child->floating)
            {
                child->render(primitive);
            }
        }
        primitive.popOrigin();
        this->drawScrollbars(primitive);
        primitive.popOrigin();
    }

    void UIView::triggerPointerDrag(float x, float y, int button)
    {
        if (button != 0)
        {
            return;
        }
        float lx = x;
        float ly = y;
        if (shape.scale.x != 0.0f && shape.scale.y != 0.0f)
        {
            Vector2 layoutPoint = shape.toUnscaled(Vector2(x, y));
            lx = layoutPoint.x;
            ly = layoutPoint.y;
        }
        Vector2 point(lx, ly);
        if (mDrag == BarDrag::None)
        {
            if (mShowV)
            {
                Rect thumb = this->verticalThumb();
                if (thumb.contains(point))
                {
                    mDrag = BarDrag::VerticalThumb;
                    mGrab = ly - thumb.origin.y;
                }
            }
            if (mDrag == BarDrag::None && mShowH)
            {
                Rect thumb = this->horizontalThumb();
                if (thumb.contains(point))
                {
                    mDrag = BarDrag::HorizontalThumb;
                    mGrab = lx - thumb.origin.x;
                }
            }
            if (mDrag == BarDrag::None && mShowV)
            {
                Rect track = this->verticalTrack();
                if (track.contains(point))
                {
                    Rect thumb = this->verticalThumb();
                    mScrollY += (ly < thumb.origin.y) ? -mInnerH : mInnerH;
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
                    mScrollX += (lx < thumb.origin.x) ? -mInnerW : mInnerW;
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
            float thumb = thumbSpan(track.size.y, mMinScrollY, mMaxScrollY);
            float travel = track.size.y - thumb;
            float t = (travel > 0.0f) ? (ly - mGrab - track.origin.y) / travel : 0.0f;
            mScrollY = clampf(mMinScrollY + t * (mMaxScrollY - mMinScrollY), mMinScrollY, mMaxScrollY);
            this->placeRoot();
        }
        else if (mDrag == BarDrag::HorizontalThumb)
        {
            Rect track = this->horizontalTrack();
            float thumb = thumbSpan(track.size.x, mMinScrollX, mMaxScrollX);
            float travel = track.size.x - thumb;
            float t = (travel > 0.0f) ? (lx - mGrab - track.origin.x) / travel : 0.0f;
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
