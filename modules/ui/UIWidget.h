#ifndef _EOKAS_UI_WIDGET_H_
#define _EOKAS_UI_WIDGET_H_

#include "UIKey.h"
#include "UIPrimitive.h"
#include "UIShape.h"
#include "UIStyle.h"
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace eokas
{
    class UIWidget
    {
    public:
        // origin is the pivot in the parent widget's local space.
        // Default pivot is the center. layout writes size and origin, and keeps pivot and scale.
        // Children stay in unscaled layout space. (0, 0) is left() / top().
        UIShape shape;
        Color color { Color(1.0f, 1.0f, 1.0f, 1.0f) };
        bool visible = true;
        bool floating = false;
        bool pickable = true;
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
        Rect visualRect() const;
        virtual void layout(const Rect& rect);
        virtual void render(UIPrimitive& primitive);
        virtual bool contains(const Vector2& point) const;
        virtual UIWidget* pick(const Vector2& point);

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
