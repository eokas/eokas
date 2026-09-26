#ifndef _EOKAS_UI_WIDGET_H_
#define _EOKAS_UI_WIDGET_H_

#include "UIPrimitive.h"
#include "UIStyle.h"
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace eokas
{
    enum class UIKey
    {
        Backspace,
        Delete,
        Left,
        Right,
        Up,
        Down,
        Home,
        End,
        Enter,
        Escape,
        A,
        C,
        X,
        V
    };

    struct UIKeyMods
    {
        bool ctrl = false;
        bool shift = false;
    };

    class UIWidget
    {
    public:
        // Position and size in the parent widget's local space.
        // localScale scales around rect's top-left. (1, 1) is identity.
        // Layout keeps rect.size unscaled. Children inherit this scale.
        Rect rect;
        Vector2 localScale { 1.0f, 1.0f };
        Color fill { Color(1.0f, 1.0f, 1.0f, 1.0f) };
        bool visible = true;
        bool floating = false;
        bool interactive = true;
        bool dragable = false;
        bool hovered = false;
        bool pressed = false;
        bool focused = false;
        std::vector<std::shared_ptr<UIWidget>> children;

        std::function<void()> onPointerEnter;
        std::function<void()> onPointerLeave;
        std::function<void()> onPointerPress;
        std::function<void()> onPointerRelease;
        std::function<void(float, float, int)> onPointerDrag;
        std::function<void()> onClick;
        std::function<void()> onDoubleClick;
        

        virtual ~UIWidget() = default;
        Rect finalRect() const;
        virtual void layout(const Rect& rect);
        virtual void render(UIPrimitive& primitive);

        virtual void triggerPointerEnter();
        virtual void triggerPointerLeave();
        virtual void triggerPointerPress();
        virtual void triggerPointerRelease();
        virtual void triggerPointerDrag(float x, float y, int button);
        virtual void triggerClick();
        virtual void resetPointerState();
        virtual bool acceptsKeyFocus() const { return false; }
        virtual void triggerFocus();
        virtual void triggerBlur();
        virtual void triggerChar(uint32_t codepoint);
        virtual void triggerKey(UIKey key, const UIKeyMods& mods);

    private:
        std::optional<std::chrono::steady_clock::time_point> mLastClickTime;
    };
}

#endif//_EOKAS_UI_WIDGET_H_
