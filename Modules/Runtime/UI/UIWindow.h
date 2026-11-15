#ifndef _EOKAS_UI_WINDOW_H_
#define _EOKAS_UI_WINDOW_H_

#include "UIEvent.h"
#include "UIWidget.h"
#include "UIPrimitive.h"
#include <memory>
#include <vector>

namespace eokas
{
    class UIWindow
    {
    public:
        void init(uint32_t width, uint32_t height);
        void quit();

        const UIWidget::Ref& root() const;
        void setRoot(const UIWidget::Ref& widget);

        void tick(f32_t deltaTime);
        UIPrimitive::Ref primitive() const;

        UIWidget* hitTest(float x, float y);
        void onMouseMove(float x, float y);
        void onMouseDown(float x, float y, UIPointerCode button);
        void onMouseUp(float x, float y, UIPointerCode button);
        void onMouseWheel(float x, float y, float deltaX, float deltaY);
        void onKeyPress(const UIKeyEvent& event);
        void onKeyRelease(const UIKeyEvent& event);
        void onKeyChar(uint32_t character);
        void setFocus(UIWidget* widget);
        UIWidget* focus() const;
        UIWidget* pressed() const;
        void clearPressed();

    private:
        void dispatchPointerPressAnywhere(float x, float y, UIPointerCode button);
        void dispatchPointerReleaseAnywhere(float x, float y, UIPointerCode button);
        void dispatchDrag(float x, float y);
        void dispatchPointerMove(UIWidget* widget, float x, float y);
        void updateHover(float x, float y);
        void resetPointerState(UIWidget* widget);
        bool focusAlive();
        bool widgetAlive(const UIWidget* widget) const;
        void refreshPointerTargets();
        void appendRawInput(const UIInputInfo& info);
        void clearRawInputEvents();
        void dispatchPointerGesture(UIWidget* pressed, UIWidget* hit, UIPointerCode button, float x, float y);
        bool pointerDragged(const std::vector<UIInputInfo>& events) const;
        bool pointerPressOrigin(const std::vector<UIInputInfo>& events, float& x, float& y) const;
        const UIInputInfo* findPointerPress(const std::vector<UIInputInfo>& events) const;
        bool isClickGesture(const std::vector<UIInputInfo>& events, UIWidget* pressed, UIWidget* hit, UIPointerCode button) const;
        bool isDragGesture(const std::vector<UIInputInfo>& events) const;
        Vector2 dropDelta(const std::vector<UIInputInfo>& events) const;

        float mWidth = 0.0f;
        float mHeight = 0.0f;

        UIWidget::Ref mRoot;
        UIPrimitive::Ref mPrimitive;

        UIWidget* mHovered = nullptr;
        UIWidget* mPressed = nullptr;
        float mPointerX = 0.0f;
        float mPointerY = 0.0f;
        bool mHasPointer = false;
        UIPointerCode mPressedButton = UIPointerCode::Unknown;
        std::vector<UIInputInfo> mRawInputEvents;
        Vector2 mLastParentPivot { 0.0f, 0.0f };
        UIWidget* mLastWidget = nullptr;
        bool mHasParentPivot = false;
        UIWidget* mFocused = nullptr;
    };
}

#endif//_EOKAS_UI_WINDOW_H_
