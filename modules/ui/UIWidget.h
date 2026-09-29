#ifndef _EOKAS_UI_WIDGET_H_
#define _EOKAS_UI_WIDGET_H_

#include "UIEvent.h"
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
        
        // parent() is the widget whose pivot space this widget is placed in.
        // Null means the parent space is the screen.
        // shape.origin is this widget's pivot relative to the parent pivot.
        // Geometry is rasterized in local space with the origin at the top-left.
        UIShape shape;

        Color color { Color(1.0f, 1.0f, 1.0f, 1.0f) };
        
        bool visible = true;
        bool floating = false;
        bool pickable = true;
        bool dragable = false;

        bool isHovered() const { return mHovered; }
        bool isPressed() const { return mPressed; }
        bool isFocused() const { return mFocused; }

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
        UIWidget* parent() const { return mParent; }
        const std::vector<Ref>& children() const { return mChildren; }
        Ref attachChild(Ref child);
        Ref detachChild(UIWidget* child);
        void detachChildren();
        Vector2 screenToPivot(const Vector2& screen) const;
        Vector2 pivotToScreen(const Vector2& pivot) const;
        Vector2 localToScreen(const Vector2& local) const;
        Vector2 screenToLocal(const Vector2& screen) const;
        Rect screenBounds() const;
        void addQuad(UIPrimitive& primitive, const Rect& local, const Rect& uv, const Color& color) const;
        virtual void layout();
        virtual void render(UIPrimitive& primitive);
        virtual bool contains(const Vector2& point) const;
        virtual UIWidget* pick(const Vector2& point);

        virtual bool handlePointerEnter(float screenX, float screenY);
        virtual bool handlePointerLeave(float screenX, float screenY);
        virtual bool handlePointerMove(float screenX, float screenY, const Vector2& delta);
        virtual bool handlePointerPress(float screenX, float screenY, int button);
        virtual bool handlePointerRelease(float screenX, float screenY, int button);
        virtual void handleFocusGain();
        virtual void handleFocusLoss();
        virtual bool handleClick(float screenX, float screenY);
        virtual bool handleDoubleClick(float screenX, float screenY);
        virtual bool handleDrag(float screenX, float screenY, const Vector2& delta);
        virtual bool handleDrop(float screenX, float screenY, UIWidget* hitUnderCursor, const Vector2& delta);
        virtual bool handleKeyPress(const UIKey& key, const UIKeyMods& mods);
        virtual bool handleKeyRelease(const UIKey& key, const UIKeyMods& mods);
        virtual bool handleWheel(float screenX, float screenY, float deltaX, float deltaY);

        virtual void endActiveDrag();

        bool containsDescendant(const UIWidget* target) const;
        void resetPointerStateRecursive();

        void resize(const Vector2& newSize);
        void placeChild(UIWidget& child, const Vector2& topLeftLocal);
        void placeChild(UIWidget& child, const Vector2& topLeftLocal, const Vector2& childSize);

    protected:
        Matrix3 matrixLocalToScreen() const;
        Matrix3 matrixScreenToLocal() const;
        Matrix3 matrixPivotToScreen() const;
        Matrix3 matrixScreenToPivot() const;

    private:
        bool isUnder(const UIWidget* ancestor) const;
        UIWidget* mParent = nullptr;
        std::vector<Ref> mChildren;
        std::optional<std::chrono::steady_clock::time_point> mLastClickTime;
        bool mHovered = false;
        bool mPressed = false;
        bool mFocused = false;
    };
}

#endif//_EOKAS_UI_WIDGET_H_
