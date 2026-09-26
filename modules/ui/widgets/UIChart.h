#ifndef _EOKAS_UI_CHART_H_
#define _EOKAS_UI_CHART_H_

#include "UIText.h"
#include <vector>

namespace eokas
{
    class UIChart : public UIWidget
    {
    public:
        float minScale = 0.25f;
        float maxScale = 4.0f;
        float scaleSensitivity = 0.002f;
        float labelPadding = 8.0f;
        Color background { Color(0.24f, 0.26f, 0.32f, 1.0f) };
        Color hoverFill { Color(0.32f, 0.44f, 0.68f, 1.0f) };
        Color pressedFill { Color(0.18f, 0.28f, 0.50f, 1.0f) };
        UIStrokeStyle stroke;
        bool selected = false;

        std::function<void(UIChart* target, float x, float y)> onDrop;
        std::function<void(float delta, float x, float y)> onZoom;

        UIChart();
        void setContour(const std::vector<Vector2>& points);
        const std::vector<Vector2>& contour() const { return mContour; }
        bool contains(const Vector2& point) const override;
        UIWidget* pick(const Vector2& point) override;
        void addChart(const std::shared_ptr<UIChart>& chart);
        void removeChart(UIChart* chart);
        void scaleAt(const Vector2& focal, float value);
        void setText(const String& value);
        UIText* label() const { return mLabel.get(); }
        void render(UIPrimitive& primitive) override;

    private:
        std::vector<Vector2> mContour;
        std::shared_ptr<UIText> mLabel;

    protected:
        Color activeFill() const;
        void placeLabel();
        void strokeLoop(UIPrimitive& primitive, const std::vector<Vector2>& localPoints) const;
    };
}

#endif//_EOKAS_UI_CHART_H_
