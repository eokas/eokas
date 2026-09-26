#ifndef _EOKAS_UI_LINK_H_
#define _EOKAS_UI_LINK_H_

#include "../widgets/UIChart.h"
#include <vector>

namespace eokas
{
    enum class UIPathKind
    {
        Straight,
        Polyline,
        CubicBezier
    };

    enum class UIAnchor
    {
        Auto,
        Center,
        Top,
        Right,
        Bottom,
        Left
    };

    struct UILinkEnd
    {
        UIChart* target = nullptr;
        UIAnchor anchor = UIAnchor::Auto;
        UIEndpointStyle cap;
    };

    class UILink : public UIChart
    {
    public:
        UIPathKind kind = UIPathKind::Straight;
        UIStrokeStyle line;
        UILinkEnd start;
        UILinkEnd end;
        float hitSlop = 6.0f;
        int bezierSteps = 16;

        UILink();
        void setPoints(const std::vector<Vector2>& parentPoints);
        bool contains(const Vector2& point) const override;
        void render(UIPrimitive& primitive) override;

    private:
        std::vector<Vector2> mLocal;

        void resolve(std::vector<Vector2>& parent) const;
        void syncBounds(const std::vector<Vector2>& parent);
    };
}

#endif//_EOKAS_UI_LINK_H_
