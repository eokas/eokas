#include "UIWidget.h"

namespace eokas
{
    Rect UIWidget::finalRect() const
    {
        return Rect(rect.origin, rect.size * localScale);
    }

    void UIWidget::layout(const Rect& rect)
    {
        this->rect = rect;
        for (auto& child : children)
        {
            if (!child)
            {
                continue;
            }
            child->layout(child->rect);
        }
    }

    void UIWidget::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Vector2 childOrigin = primitive.origin() + primitive.scale() * rect.origin;
        Vector2 childScale = primitive.scale() * localScale;
        primitive.pushTransform(childOrigin, childScale);
        for (auto& child : children)
        {
            if (child && !child->floating && !primitive.outsideClip(child->finalRect()))
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
