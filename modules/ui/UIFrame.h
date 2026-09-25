#ifndef _EOKAS_UI_FRAME_H_
#define _EOKAS_UI_FRAME_H_

#include "UIWidget.h"
#include "UIShape.h"
#include <memory>

namespace eokas
{
    class UIFrame
    {
    public:
        void init(uint32_t width, uint32_t height);
        void quit();

        const std::shared_ptr<UIWidget>& root() const;
        void setRoot(const std::shared_ptr<UIWidget>& widget);

        void flush();
        UIShape::Ref shape() const;

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
        UIWidget* hitTestNode(UIWidget* widget, float x, float y, float originX, float originY);
        bool findWidget(UIWidget* node, UIWidget* target, float originX, float originY, float& outX, float& outY) const;
        void dispatchDrag(float x, float y);
        bool routeWheel(UIWidget* widget, float x, float y, float originX, float originY, float deltaX, float deltaY);
        void resetPointerState(UIWidget* widget);
        bool containsWidget(UIWidget* node, UIWidget* target) const;
        bool focusAlive();

        float mWidth = 0.0f;
        float mHeight = 0.0f;

        std::shared_ptr<UIWidget> mRoot;
        UIShape::Ref mShape;

        UIWidget* mHovered = nullptr;
        UIWidget* mPressed = nullptr;
        int mPressedButton = -1;
        UIWidget* mFocused = nullptr;
    };
}

#endif//_EOKAS_UI_FRAME_H_
