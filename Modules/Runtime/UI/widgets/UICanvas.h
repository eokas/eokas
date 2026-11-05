#ifndef _EOKAS_UI_CANVAS_H_
#define _EOKAS_UI_CANVAS_H_

#include "../UIWidget.h"

namespace eokas
{
    class UIChart;
    class UICanvas : public UIWidget
    {
        friend class UIChart;
    public:
        float minScale = 0.25f;
        float maxScale = 4.0f;
        float scaleSensitivity = 0.002f;

        UICanvas();
        void scaleAt(const Vector2& focal, float value);
        void layout() override;
        void render(UIPrimitive& primitive) override;
        void refit();
        void dragChild(UIWidget* widget, float parentPivotX, float parentPivotY);
        void endDrag();

        bool handlePointerMove(const UIPointerEvent& event) override;
        bool handlePointerRelease(const UIPointerEvent& event) override;
        bool handleDrag(float screenX, float screenY, const Vector2& delta) override;
        bool handleDrop(float screenX, float screenY, UIWidget* hitUnderCursor, const Vector2& delta) override;
        bool handleWheel(float screenX, float screenY, float deltaX, float deltaY) override;
        void endActiveDrag() override;

    private:
        UIWidget* findPressedDragable(UIWidget* node) const;
        void applyCanvasSelection(UIWidget* selectedItem) const;

        bool mSelfDrag = false;
        UIWidget* mDragWidget = nullptr;
        Vector2 mGrab { 0.0f, 0.0f };
    };
}

#endif//_EOKAS_UI_CANVAS_H_
