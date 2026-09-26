#include "UIWidget.h"

namespace eokas
{
    Rect UIWidget::visualRect() const
    {
        return shape.visualRect();
    }

    bool UIWidget::contains(const Vector2& point) const
    {
        Rect box(shape.left(), shape.top(), shape.size.x, shape.size.y);
        return box.contains(point);
    }

    UIWidget* UIWidget::pick(const Vector2& point)
    {
        if (!visible || shape.scale.x == 0.0f || shape.scale.y == 0.0f)
        {
            return nullptr;
        }
        Vector2 local = shape.toLocal(point);
        bool inside = this->contains(Vector2(shape.left(), shape.top()) + local);
        for (auto it = children.rbegin(); it != children.rend(); ++it)
        {
            if (*it && (*it)->floating)
            {
                if (UIWidget* hit = (*it)->pick(local))
                {
                    return hit;
                }
            }
        }
        for (auto it = children.rbegin(); it != children.rend(); ++it)
        {
            if (*it && !(*it)->floating)
            {
                if (UIWidget* hit = (*it)->pick(local))
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

    void UIWidget::layout(const Rect& rect)
    {
        shape.size = rect.size;
        shape.origin = rect.origin + shape.pivot * shape.size;
        for (auto& child : children)
        {
            if (!child)
            {
                continue;
            }
            child->layout(Rect(child->shape.left(), child->shape.top(), child->shape.size.x, child->shape.size.y));
        }
    }

    void UIWidget::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Vector2 childOrigin = primitive.origin() + primitive.scale() * shape.toParent(Vector2(shape.left(), shape.top()));
        Vector2 childScale = primitive.scale() * shape.scale;
        primitive.pushTransform(childOrigin, childScale);
        for (auto& child : children)
        {
            if (child && !child->floating && !primitive.outsideClip(child->visualRect()))
            {
                child->render(primitive);
            }
        }
        for (auto& child : children)
        {
            if (child && child->floating)
            {
                child->render(primitive);
            }
        }
        primitive.popOrigin();
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
