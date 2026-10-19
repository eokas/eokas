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

        const std::shared_ptr<UIWidget>& root() const;
        void setRoot(const std::shared_ptr<UIWidget>& widget);

        void tick(f32_t deltaTime);
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
        void dispatchDrag(float x, float y);
        void dispatchPointerMove(UIWidget* widget, float x, float y);
        void resetPointerState(UIWidget* widget);
        bool focusAlive();
        void appendRawInput(const UIInputInfo& info);
        void clearRawInputEvents();
        void dispatchPointerGesture(UIWidget* pressed, UIWidget* hit, int button, float x, float y);
        bool pointerDragged(const std::vector<UIInputInfo>& events) const;
        bool pointerPressOrigin(const std::vector<UIInputInfo>& events, float& x, float& y) const;
        const UIInputInfo* findPointerPress(const std::vector<UIInputInfo>& events) const;
        bool isClickGesture(const std::vector<UIInputInfo>& events, UIWidget* pressed, UIWidget* hit, int button) const;
        bool isDragGesture(const std::vector<UIInputInfo>& events) const;
        Vector2 dropDelta(const std::vector<UIInputInfo>& events) const;

        float mWidth = 0.0f;
        float mHeight = 0.0f;

        std::shared_ptr<UIWidget> mRoot;
        UIPrimitive::Ref mPrimitive;

        UIWidget* mHovered = nullptr;
        UIWidget* mPressed = nullptr;
        int mPressedButton = -1;
        std::vector<UIInputInfo> mRawInputEvents;
        Vector2 mLastParentPivot { 0.0f, 0.0f };
        UIWidget* mLastWidget = nullptr;
        bool mHasParentPivot = false;
        UIWidget* mFocused = nullptr;
    };
}

#endif//_EOKAS_UI_WINDOW_H_
