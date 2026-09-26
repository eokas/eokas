#ifndef _EOKAS_UI_FRAME_H_
#define _EOKAS_UI_FRAME_H_

#include "UIWidget.h"
#include "UIPrimitive.h"
#include <memory>
#include <vector>

namespace eokas
{
    class UICanvas;

    class UIFrame
    {
    public:
        void init(uint32_t width, uint32_t height);
        void quit();

        const std::shared_ptr<UIWidget>& root() const;
        void setRoot(const std::shared_ptr<UIWidget>& widget);

        void flush();
        UIPrimitive::Ref primitive() const;

        UIWidget* hitTest(float x, float y);
        void onMouseMove(float x, float y);
        void onMouseDown(float x, float y, int button);
        void onMouseUp(float x, float y, int button);
        void onMouseWheel(float x, float y, float deltaX, float deltaY);
        void onChar(uint32_t codepoint);
        void onKeyDown(UIKey key, const UIKeyMods& mods);
        void setFocus(UIWidget* widget);
        UIWidget* focus() const;

    private:
        bool findWidget(UIWidget* node, UIWidget* target, const Vector2& origin, const Vector2& scale, Vector2& outOrigin, Vector2& outScale) const;
        bool collectPath(UIWidget* node, UIWidget* target, std::vector<UIWidget*>& path) const;
        UIWidget* dragTargetOf(UIWidget* pressed, UICanvas*& canvas) const;
        void dispatchDrag(float x, float y);
        bool routeWheel(UIWidget* widget, const Vector2& point, const Vector2& origin, const Vector2& scale, const Vector2& delta);
        bool routeNestedCanvas(UIWidget* widget, const Vector2& point, const Vector2& origin, const Vector2& scale, const Vector2& delta);
        void endCanvasDrag(UIWidget* widget);
        void resetPointerState(UIWidget* widget);
        bool containsWidget(UIWidget* node, UIWidget* target) const;
        bool focusAlive();

        float mWidth = 0.0f;
        float mHeight = 0.0f;

        std::shared_ptr<UIWidget> mRoot;
        UIPrimitive::Ref mPrimitive;

        UIWidget* mHovered = nullptr;
        UIWidget* mPressed = nullptr;
        int mPressedButton = -1;
        float mPressX = 0.0f;
        float mPressY = 0.0f;
        bool mDragged = false;
        UIWidget* mFocused = nullptr;
    };
}

#endif//_EOKAS_UI_FRAME_H_
