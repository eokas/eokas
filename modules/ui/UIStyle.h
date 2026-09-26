#ifndef _EOKAS_UI_STYLE_H_
#define _EOKAS_UI_STYLE_H_

#include "header.h"

namespace eokas
{
    enum class UILinePattern
    {
        Solid,
        Dashed
    };

    struct UIStrokeStyle
    {
        Color color { 0.95f, 0.97f, 1.0f, 1.0f };
        float thickness = 1.0f;
        UILinePattern pattern = UILinePattern::Solid;
        float dashLength = 8.0f;
        float gapLength = 4.0f;

        UIStrokeStyle() = default;
        UIStrokeStyle(const Color& value) : color(value) {}
        UIStrokeStyle& operator=(const Color& value)
        {
            color = value;
            return *this;
        }
    };

    struct UITextStyle
    {
        String fontPath;
        float fontSize = 16.0f;
        Color color { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    enum class UIEndpointKind
    {
        None,
        ArrowFilled,
        ArrowHollow,
        TriangleFilled,
        TriangleHollow,
        CircleFilled,
        CircleHollow,
        DiamondFilled,
        DiamondHollow
    };

    struct UIEndpointStyle
    {
        UIEndpointKind kind = UIEndpointKind::None;
        float size = 12.0f;
        Color fill { Color(0.95f, 0.97f, 1.0f, 1.0f) };
        UIStrokeStyle stroke;
    };
}

#endif//_EOKAS_UI_STYLE_H_
