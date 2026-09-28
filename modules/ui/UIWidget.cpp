#include "UIWidget.h"

namespace eokas
{
    UIWidget::~UIWidget()
    {
        for (auto& child : mChildren)
        {
            if (child && child->mParent == this)
            {
                child->mParent = nullptr;
            }
        }
    }

    bool UIWidget::isUnder(const UIWidget* ancestor) const
    {
        for (const UIWidget* cursor = this; cursor != nullptr; cursor = cursor->mParent)
        {
            if (cursor == ancestor)
            {
                return true;
            }
        }
        return false;
    }

    UIWidget::Ref UIWidget::attachChild(UIWidget::Ref child)
    {
        if (!child || this->isUnder(child.get()))
        {
            return nullptr;
        }
        if (child->mParent == this)
        {
            return child;
        }
        if (child->mParent != nullptr)
        {
            child->mParent->detachChild(child.get());
        }
        child->mParent = this;
        mChildren.push_back(child);
        return child;
    }

    UIWidget::Ref UIWidget::detachChild(UIWidget* child)
    {
        if (child == nullptr)
        {
            return nullptr;
        }
        for (auto it = mChildren.begin(); it != mChildren.end(); ++it)
        {
            if (it->get() == child)
            {
                Ref held = *it;
                mChildren.erase(it);
                if (held->mParent == this)
                {
                    held->mParent = nullptr;
                }
                return held;
            }
        }
        return nullptr;
    }

    void UIWidget::detachChildren()
    {
        for (auto& child : mChildren)
        {
            if (child && child->mParent == this)
            {
                child->mParent = nullptr;
            }
        }
        mChildren.clear();
    }

    Matrix3 UIWidget::matrixLocalToScreen() const
    {
        Matrix3 local = shape.localTrans();
        if (mParent == nullptr)
        {
            return local;
        }
        return Matrix3::transform(local, mParent->matrixPivotToScreen());
    }

    Matrix3 UIWidget::matrixScreenToLocal() const
    {
        return this->matrixLocalToScreen().inverse();
    }

    Matrix3 UIWidget::matrixPivotToScreen() const
    {
        Matrix3 shift = Matrix3::translation(Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y));
        return Matrix3::transform(shift, this->matrixLocalToScreen());
    }

    Matrix3 UIWidget::matrixScreenToPivot() const
    {
        return this->matrixPivotToScreen().inverse();
    }

    Vector2 UIWidget::screenToPivot(const Vector2& screen) const
    {
        return UIShape::transformPoint(this->matrixScreenToPivot(), screen);
    }

    Vector2 UIWidget::pivotToScreen(const Vector2& pivot) const
    {
        return UIShape::transformPoint(this->matrixPivotToScreen(), pivot);
    }

    Vector2 UIWidget::localToScreen(const Vector2& local) const
    {
        return UIShape::transformPoint(this->matrixLocalToScreen(), local);
    }

    Vector2 UIWidget::screenToLocal(const Vector2& screen) const
    {
        return UIShape::transformPoint(this->matrixScreenToLocal(), screen);
    }

    Rect UIWidget::screenBounds() const
    {
        return shape.bounds(this->matrixLocalToScreen());
    }

    void UIWidget::addQuad(UIPrimitive& primitive, const Rect& local, const Rect& uv, const Color& color) const
    {
        primitive.addQuad(this->matrixLocalToScreen(), local, uv, color);
    }

    void UIWidget::layout()
    {
        if (!visible)
        {
            return;
        }
        for (auto& child : mChildren)
        {
            if (child)
            {
                child->layout();
            }
        }
    }

    void UIWidget::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        for (auto& child : mChildren)
        {
            if (!child || child->floating)
            {
                continue;
            }
            if (!primitive.outsideClip(child->shape.bounds(child->matrixLocalToScreen())))
            {
                child->render(primitive);
            }
        }
        for (auto& child : mChildren)
        {
            if (child && child->floating)
            {
                child->render(primitive);
            }
        }
    }

    bool UIWidget::contains(const Vector2& point) const
    {
        return point.x >= 0.0f && point.y >= 0.0f
            && point.x <= shape.size.x && point.y <= shape.size.y;
    }

    UIWidget* UIWidget::pick(const Vector2& point)
    {
        if (!visible || shape.scale.x == 0.0f || shape.scale.y == 0.0f)
        {
            return nullptr;
        }
        Vector2 local = shape.pivotToLocal(point);
        bool inside = this->contains(local);
        Vector2 childPoint = local - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
        for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
        {
            if (*it && (*it)->floating)
            {
                if (UIWidget* hit = (*it)->pick(childPoint))
                {
                    return hit;
                }
            }
        }
        for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
        {
            if (*it && !(*it)->floating)
            {
                if (UIWidget* hit = (*it)->pick(childPoint))
                {
                    return hit;
                }
            }
        }
        if (pickable && inside)
        {
            return this;
        }
        return nullptr;
    }

    void UIWidget::triggerPointerEnter()
    {
        hovered = true;
        if (onPointerEnter)
        {
            onPointerEnter();
        }
    }

    void UIWidget::triggerPointerLeave()
    {
        hovered = false;
        if (onPointerLeave)
        {
            onPointerLeave();
        }
    }

    void UIWidget::triggerPointerPress()
    {
        pressed = true;
        if (onPointerPress)
        {
            onPointerPress();
        }
    }

    void UIWidget::triggerPointerRelease()
    {
        const bool click = pressed;
        pressed = false;
        if (onPointerRelease)
        {
            onPointerRelease();
        }
        if (!click)
        {
            return;
        }

        if (onClick)
        {
            onClick();
        }

        constexpr auto interval = std::chrono::milliseconds(500);
        auto now = std::chrono::steady_clock::now();
        if (mLastClickTime.has_value() && now - *mLastClickTime < interval)
        {
            mLastClickTime.reset();
            if (onDoubleClick)
            {
                onDoubleClick();
            }
            return;
        }
        mLastClickTime = now;
    }

    void UIWidget::triggerPointerMove(const Vector2& position, const Vector2& delta)
    {
        if (onPointerMove)
        {
            onPointerMove(position, delta);
        }
        if (pressed && onDrag)
        {
            onDrag(position, delta);
        }
    }

    void UIWidget::triggerWheel(const Vector2& position, f32_t delta)
    {
        if (onWheel)
        {
            onWheel(position, delta);
        }
    }

    void UIWidget::triggerKeyPress(const UIKey& key, const UIKeyMods& mods)
    {
        if (onKeyPress)
        {
            onKeyPress(key, mods);
        }
    }

    void UIWidget::triggerKeyRelease(const UIKey& key, const UIKeyMods& mods)
    {
        if (onKeyRelease)
        {
            onKeyRelease(key, mods);
        }
    }

    void UIWidget::resize(const Vector2& newSize)
    {
        Vector2 shift = Vector2(
            shape.pivot.x * (newSize.x - shape.size.x),
            shape.pivot.y * (newSize.y - shape.size.y));
        shape.origin += UIShape::transformVector(shape.localTrans(), shift);
        shape.size = newSize;
        for (auto& child : mChildren)
        {
            if (child)
            {
                child->shape.origin -= shift;
            }
        }
    }

    void UIWidget::placeChild(UIWidget& child, const Vector2& topLeftLocal)
    {
        Vector2 topLeftPivot = topLeftLocal - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
        child.shape.setBox(topLeftPivot, child.shape.size);
    }

    void UIWidget::placeChild(UIWidget& child, const Vector2& topLeftLocal, const Vector2& childSize)
    {
        child.shape.size = childSize;
        this->placeChild(child, topLeftLocal);
    }
}
