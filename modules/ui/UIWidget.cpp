#include "UIWidget.h"

namespace eokas
{
    UIWidget::~UIWidget()
    {
        for (auto& child : children)
        {
            if (child && child->shape.parent == &shape)
            {
                child->shape.parent = nullptr;
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

    void UIWidget::placeChild(UIWidget& child, const Vector2& topLeftLocal)
    {
        child.shape.parent = &shape;
        Vector2 topLeftPivot = topLeftLocal - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
        child.shape.setBox(topLeftPivot, child.shape.size);
    }

    void UIWidget::placeChild(UIWidget& child, const Vector2& topLeftLocal, const Vector2& childSize)
    {
        child.shape.size = childSize;
        this->placeChild(child, topLeftLocal);
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

    void UIWidget::bindChildren()
    {
        for (auto& child : children)
        {
            if (!child)
            {
                continue;
            }
            child->shape.parent = &shape;
            child->bindChildren();
        }
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
            child->shape.parent = &shape;
            if (!primitive.outsideClip(child->shape.bounds(child->shape.worldTrans())))
            {
                child->render(primitive);
            }
        }
        for (auto& child : children)
        {
            if (child && child->floating)
            {
                child->shape.parent = &shape;
                child->render(primitive);
            }
        }
    }

    void UIWidget::triggerPointerDrag(float x, float y, int button)
    {
        if (onPointerDrag)
        {
            onPointerDrag(x, y, button);
        }
    }

    void UIWidget::triggerFocus()
    {
        focused = true;
    }

    void UIWidget::triggerBlur()
    {
        focused = false;
    }

    void UIWidget::triggerChar(uint32_t codepoint)
    {
        (void)codepoint;
    }

    void UIWidget::triggerKey(UIKey key, const UIKeyMods& mods)
    {
        (void)key;
        (void)mods;
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
        pressed = false;
        if (onPointerRelease)
        {
            onPointerRelease();
        }
    }

    void UIWidget::triggerClick()
    {
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

    void UIWidget::resetPointerState()
    {
        hovered = false;
        pressed = false;
        mLastClickTime.reset();
    }
}
