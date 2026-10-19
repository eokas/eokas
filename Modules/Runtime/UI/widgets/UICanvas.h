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
        void addChild(const std::shared_ptr<UIWidget>& child);
        void layout() override;
        void render(UIPrimitive& primitive) override;
        void refit();
        void dragChild(UIWidget* widget, float localX, float localY);
        void endDrag();

        bool handlePointerMove(float screenX, float screenY, const Vector2& delta) override;
        bool handlePointerRelease(float screenX, float screenY, int button) override;
        bool handleDrag(float screenX, float screenY, const Vector2& delta) override;
        bool handleDrop(float screenX, float screenY, UIWidget* hitUnderCursor, const Vector2& delta) override;
        bool handleWheel(float screenX, float screenY, float deltaX, float deltaY) override;
        void endActiveDrag() override;

    private:
        UIWidget* findPressedDragable(UIWidget* node) const;
        void applyCanvasSelection(UIWidget* selectedItem) const;
        bool routeNestedWheel(float screenX, float screenY, float deltaX, float deltaY, UIWidget* widget) const;

        bool mSelfDrag = false;
        UIWidget* mDragWidget = nullptr;
        Vector2 mGrab { 0.0f, 0.0f };
    };
}

#endif//_EOKAS_UI_CANVAS_H_
