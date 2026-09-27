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
        using Ref = std::shared_ptr<UIWidget>;
        
        // parent is the widget whose pivot space this widget is placed in.
        // Null means the parent space is the screen.
        // shape.origin is this widget's pivot relative to the parent pivot.
        // Geometry is rasterized in local space with the origin at the top-left, then multiplied by worldTrans().
        UIWidget* parent = nullptr;
        UIShape shape;

        Color color { Color(1.0f, 1.0f, 1.0f, 1.0f) };
        
        bool visible = true;
        bool floating = false;
        bool pickable = true;
        bool dragable = false;

        bool hovered = false;
        bool pressed = false;
        bool focused = false;

        std::vector<UIWidget::Ref> children;

        std::function<void()> onPointerEnter;
        std::function<void()> onPointerLeave;
        std::function<void()> onPointerPress;
        std::function<void()> onPointerRelease;
        std::function<void(const Vector2& position, const Vector2& delta)> onPointerMove;
        std::function<void(const Vector2& position, f32_t delta)> onWheel;
        std::function<void(const UIKey&, const UIKeyMods&)> onKeyPress;
        std::function<void(const UIKey&, const UIKeyMods&)> onKeyRelease;

        std::function<void()> onClick;
        std::function<void()> onDoubleClick;
        std::function<void(const Vector2& position, const Vector2& delta)> onDrag;
        std::function<void(const Vector2& position, const Vector2& delta)> onDrop;
        
        std::function<void()> onGotFocus;
        std::function<void()> onLostFocus;

        virtual ~UIWidget();
        Matrix3 worldTrans() const;
        Matrix3 pivotToScreen() const;
        virtual void render(UIPrimitive& primitive);
        virtual bool contains(const Vector2& point) const;
        virtual UIWidget* pick(const Vector2& point);

        virtual void triggerPointerEnter();
        virtual void triggerPointerLeave();
        virtual void triggerPointerPress();
        virtual void triggerPointerRelease();
        virtual void triggerPointerMove(const Vector2& position, const Vector2& delta);
        virtual void triggerWheel(const Vector2& position, f32_t delta);
        virtual void triggerKeyPress(const UIKey& key, const UIKeyMods& mods);
        virtual void triggerKeyRelease(const UIKey& key, const UIKeyMods& mods);

        void resize(const Vector2& newSize);    
        void placeChild(UIWidget& child, const Vector2& topLeftLocal);
        void placeChild(UIWidget& child, const Vector2& topLeftLocal, const Vector2& childSize);
        virtual void bindChildren();
    
    private:
        std::optional<std::chrono::steady_clock::time_point> mLastClickTime;
    };
}

#endif//_EOKAS_UI_WIDGET_H_
