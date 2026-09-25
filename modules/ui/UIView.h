#ifndef _EOKAS_UI_VIEW_H_
#define _EOKAS_UI_VIEW_H_

#include "UIWidget.h"

namespace eokas
{
    class UIView : public UIWidget
    {
    public:
        float scrollbarThickness = 8.0f;
        Color scrollbarColor { 0.45f, 0.48f, 0.55f, 1.0f };
        Color scrollbarTrackColor { 0.20f, 0.21f, 0.24f, 1.0f };
        Color scrollbarPressedColor { 0.32f, 0.44f, 0.68f, 1.0f };

        UIView();
        const std::shared_ptr<UIWidget>& root() const { return mRoot; }
        void addChild(const std::shared_ptr<UIWidget>& child);
        void setScroll(float x, float y);
        float scrollX() const { return mScrollX; }
        float scrollY() const { return mScrollY; }
        bool scrollBy(float dx, float dy);
        Rect viewport() const;
        bool scrollbarContains(float localX, float localY) const;

        void layout(const Rect& rect) override;
        void render(UIShape& shape) override;
        void triggerPointerDrag(float x, float y, int button) override;
        void triggerPointerRelease() override;

    private:
        enum class BarDrag
        {
            None,
            VerticalThumb,
            HorizontalThumb,
            Track
        };

        std::shared_ptr<UIWidget> mRoot;
        float mScrollX = 0.0f;
        float mScrollY = 0.0f;
        float mMinScrollX = 0.0f;
        float mMinScrollY = 0.0f;
        float mMaxScrollX = 0.0f;
        float mMaxScrollY = 0.0f;
        float mInnerW = 0.0f;
        float mInnerH = 0.0f;
        bool mShowV = false;
        bool mShowH = false;
        BarDrag mDrag = BarDrag::None;
        float mGrab = 0.0f;

        void updateMetrics();
        void placeRoot();
        void expandContent(const UIWidget* widget, float& minX, float& minY, float& maxX, float& maxY, bool& any) const;
        float barSize() const;
        Rect verticalTrack() const;
        Rect horizontalTrack() const;
        Rect verticalThumb() const;
        Rect horizontalThumb() const;
        void drawScrollbars(UIShape& shape) const;
    };
}

#endif//_EOKAS_UI_VIEW_H_
