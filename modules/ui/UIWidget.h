#ifndef _EOKAS_UI_WIDGET_H_
#define _EOKAS_UI_WIDGET_H_

#include "UIShape.h"
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
        Rect rect;
        Color color { 1.0f, 1.0f, 1.0f, 1.0f };
        bool visible = true;
        bool floating = false;
        bool interactive = true;
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
        virtual void layout(const Rect& rect);
        virtual void render(UIShape& shape);

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
