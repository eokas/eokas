#ifndef _EOKAS_UI_CANVAS_H_
#define _EOKAS_UI_CANVAS_H_

#include "../UIWidget.h"

namespace eokas
{
    class UIChart;

    class UICanvas : public UIWidget
    {
    public:
        float minScale = 0.25f;
        float maxScale = 4.0f;
        float scaleSensitivity = 0.002f;

        UICanvas();
        void scaleAt(const Vector2& focal, float value);
        void addChild(const std::shared_ptr<UIWidget>& child);
        void layout(const Rect& rect) override;
        void render(UIPrimitive& primitive) override;
        void refit();
        void dragChild(UIWidget* widget, float localX, float localY);
        void endDrag();
        void dispatchDrop(UIChart* source, UIWidget* hit, float x, float y);
        void select(UIChart* chart);
        void triggerPointerDrag(float x, float y, int button) override;
        void triggerPointerRelease() override;

    private:
        bool mSelfDrag = false;
        UIWidget* mDragWidget = nullptr;
        Vector2 mGrab { 0.0f, 0.0f };
    };
}

#endif//_EOKAS_UI_CANVAS_H_
