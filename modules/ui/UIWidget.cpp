#include "UIWidget.h"

namespace eokas
{
    UIWidget::~UIWidget()
    {
        for (auto& child : children)
        {
            if (child && child->parent == this)
            {
                child->parent = nullptr;
            }
        }
    }

    Matrix3 UIWidget::worldTrans() const
    {
        Matrix3 local = shape.localTrans();
        if (parent == nullptr)
        {
            return local;
        }
        return Matrix3::transform(local, parent->pivotToScreen());
    }

    Matrix3 UIWidget::pivotToScreen() const
    {
        Matrix3 shift = Matrix3::translation(Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y));
        return Matrix3::transform(shift, worldTrans());
    }

    void UIWidget::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        for (auto& child : children)
        {
            if (!child || child->floating)
            {
                continue;
            }
            child->parent = this;
            if (!primitive.outsideClip(child->shape.bounds(child->worldTrans())))
            {
                child->render(primitive);
            }
        }
        for (auto& child : children)
        {
            if (child && child->floating)
            {
                child->parent = this;
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
        Vector2 local = shape.toLocal(point);
        bool inside = this->contains(local);
        Vector2 childPoint = local - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
        for (auto it = children.rbegin(); it != children.rend(); ++it)
        {
            if (*it && (*it)->floating)
            {
                if (UIWidget* hit = (*it)->pick(childPoint))
                {
                    return hit;
                }
            }
        }
        for (auto it = children.rbegin(); it != children.rend(); ++it)
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
        for (auto& child : children)
        {
            if (child)
            {
                child->shape.origin -= shift;
            }
        }
    }

    void UIWidget::placeChild(UIWidget& child, const Vector2& topLeftLocal)
    {
        child.parent = this;
        Vector2 topLeftPivot = topLeftLocal - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
        child.shape.setBox(topLeftPivot, child.shape.size);
    }

    void UIWidget::placeChild(UIWidget& child, const Vector2& topLeftLocal, const Vector2& childSize)
    {
        child.shape.size = childSize;
        this->placeChild(child, topLeftLocal);
    }

    void UIWidget::bindChildren()
    {
        for (auto& child : children)
        {
            if (!child)
            {
                continue;
            }
            child->parent = this;
            child->bindChildren();
        }
    }
}
