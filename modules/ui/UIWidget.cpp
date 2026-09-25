#include "UIWidget.h"

namespace eokas
{
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

    void UIWidget::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        for (auto& child : children)
        {
            if (child && !child->floating && !shape.outsideClip(child->rect))
            {
                child->render(shape);
            }
        }
        for (auto& child : children)
        {
            if (child && child->floating)
            {
                child->render(shape);
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
