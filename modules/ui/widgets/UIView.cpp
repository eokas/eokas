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

        Rect localToScreenRect(const Matrix3& localToScreen, const Rect& local)
        {
            Vector2 corner[4] = {
                UIShape::transformPoint(localToScreen, local.origin),
                UIShape::transformPoint(localToScreen, Vector2(local.origin.x + local.size.x, local.origin.y)),
                UIShape::transformPoint(localToScreen, local.origin + local.size),
                UIShape::transformPoint(localToScreen, Vector2(local.origin.x, local.origin.y + local.size.y))
            };
            float minX = corner[0].x;
            float minY = corner[0].y;
            float maxX = corner[0].x;
            float maxY = corner[0].y;
            for (int i = 1; i < 4; ++i)
            {
                minX = minf(minX, corner[i].x);
                minY = minf(minY, corner[i].y);
                maxX = maxf(maxX, corner[i].x);
                maxY = maxf(maxY, corner[i].y);
            }
            return Rect(minX, minY, maxX - minX, maxY - minY);
        }
    }

    UIView::UIView()
    {
        pickable = false;
        mRoot = std::make_shared<UIWidget>();
        mRoot->pickable = false;
        this->attachChild(mRoot);
    }

    void UIView::addChild(const std::shared_ptr<UIWidget>& child)
    {
        mRoot->attachChild(child);
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
        return Rect(0.0f, 0.0f, mInnerW, mInnerH);
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
        Vector2 viewLocal = shape.pivotToLocal(point);
        bool inside = this->contains(viewLocal);
        bool contentScale = mRoot && mRoot->shape.scale.x != 0.0f && mRoot->shape.scale.y != 0.0f;
        Vector2 content = Vector2::ZERO;
        if (contentScale)
        {
            Vector2 inViewPivot = viewLocal - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
            Vector2 rootLocal = mRoot->shape.pivotToLocal(inViewPivot);
            content = rootLocal - Vector2(mRoot->shape.pivot.x * mRoot->shape.size.x, mRoot->shape.pivot.y * mRoot->shape.size.y);
            for (auto it = mRoot->children().rbegin(); it != mRoot->children().rend(); ++it)
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
        if (this->scrollbarContains(viewLocal.x, viewLocal.y))
        {
            return this;
        }
        if (!this->viewport().contains(viewLocal))
        {
            if (pickable && inside)
            {
                return this;
            }
            return nullptr;
        }
        if (contentScale)
        {
            for (auto it = mRoot->children().rbegin(); it != mRoot->children().rend(); ++it)
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
        Vector2 topLeft(
            -shape.pivot.x * shape.size.x - mScrollX,
            -shape.pivot.y * shape.size.y - mScrollY);
        mRoot->shape.setBox(topLeft, mRoot->shape.size);
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
        Rect box = widget->shape.bounds(widget->shape.localTrans());
        Vector2 corners[4] = {
            box.origin,
            Vector2(box.origin.x + box.size.x, box.origin.y),
            box.origin + box.size,
            Vector2(box.origin.x, box.origin.y + box.size.y)
        };
        for (int i = 0; i < 4; ++i)
        {
            const UIWidget* node = widget->parent();
            Vector2 point = corners[i];
            while (node != nullptr && node != mRoot.get())
            {
                Vector2 local(point.x + node->shape.pivot.x * node->shape.size.x, point.y + node->shape.pivot.y * node->shape.size.y);
                point = UIShape::transformPoint(node->shape.localTrans(), local);
                node = node->parent();
            }
            Vector2 viewLocal(
                point.x + mRoot->shape.pivot.x * mRoot->shape.size.x,
                point.y + mRoot->shape.pivot.y * mRoot->shape.size.y);
            if (!any)
            {
                minX = maxX = viewLocal.x;
                minY = maxY = viewLocal.y;
                any = true;
            }
            else
            {
                minX = minf(minX, viewLocal.x);
                minY = minf(minY, viewLocal.y);
                maxX = maxf(maxX, viewLocal.x);
                maxY = maxf(maxY, viewLocal.y);
            }
        }
        if (dynamic_cast<const UIView*>(widget) != nullptr)
        {
            return;
        }
        for (auto& child : widget->children())
        {
            this->expandContent(child.get(), minX, minY, maxX, maxY, any);
        }
    }

    void UIView::updateMetrics()
    {
        bool any = false;
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        for (auto& child : mRoot->children())
        {
            this->expandContent(child.get(), minX, minY, maxX, maxY, any);
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
            mRoot->resize(Vector2::ZERO);
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
        mRoot->resize(Vector2(contentW, contentH));
        this->placeRoot();
    }

    Rect UIView::verticalTrack() const
    {
        return Rect(mInnerW, 0.0f, this->barSize(), mInnerH);
    }

    Rect UIView::horizontalTrack() const
    {
        return Rect(0.0f, mInnerH, mInnerW, this->barSize());
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

    void UIView::drawScrollbars(UIPrimitive& primitive, const Matrix3& localToScreen) const
    {
        Rect solid = UIFont::solidUV();
        if (mShowV)
        {
            primitive.addQuad(localToScreen, this->verticalTrack(), solid, scrollbarTrack);
            Color thumb = (mDrag == BarDrag::VerticalThumb) ? scrollbarPressed : scrollbar;
            primitive.addQuad(localToScreen, this->verticalThumb(), solid, thumb);
        }
        if (mShowH)
        {
            primitive.addQuad(localToScreen, this->horizontalTrack(), solid, scrollbarTrack);
            Color thumb = (mDrag == BarDrag::HorizontalThumb) ? scrollbarPressed : scrollbar;
            primitive.addQuad(localToScreen, this->horizontalThumb(), solid, thumb);
        }
        if (mShowV && mShowH)
        {
            float bar = this->barSize();
            primitive.addQuad(localToScreen, Rect(mInnerW, mInnerH, bar, bar), solid, scrollbarTrack);
        }
    }

    void UIView::layout()
    {
        if (!visible)
        {
            return;
        }
        this->updateMetrics();
        UIWidget::layout();
    }

    void UIView::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Matrix3 localToScreen = matrixLocalToScreen();
        if (color.a > 0.0f)
        {
            primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }

        primitive.pushClip(localToScreenRect(localToScreen, this->viewport()));
        for (auto& child : mRoot->children())
        {
            if (!child || child->floating)
            {
                continue;
            }
            if (!primitive.outsideClip(child->screenBounds()))
            {
                child->render(primitive);
            }
        }
        primitive.popClip();
        for (auto& child : mRoot->children())
        {
            if (child && child->floating)
            {
                child->render(primitive);
            }
        }
        this->drawScrollbars(primitive, localToScreen);
    }

    void UIView::triggerPointerMove(const Vector2& position, const Vector2& delta)
    {
        (void)delta;
        UIWidget::triggerPointerMove(position, delta);
        if (!pressed)
        {
            return;
        }
        Vector2 local = shape.pivotToLocal(position);
        float lx = local.x;
        float ly = local.y;
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
