#ifndef _EOKAS_UI_IMAGE_H_
#define _EOKAS_UI_IMAGE_H_

#include "../UIWidget.h"

namespace eokas
{
    enum class UIImageType
    {
        Simple,
        Sliced
    };

    struct UIBorder
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;

        UIBorder()
        {
        }

        UIBorder(float left, float top, float right, float bottom)
            : left(left), top(top), right(right), bottom(bottom)
        {
        }
    };

    class UIImage : public UIWidget
    {
    public:
        Rect uv { 0.0f, 0.0f, 1.0f, 1.0f };
        UIBorder border;
        UIImageType type = UIImageType::Simple;
        bool fillCenter = true;
        void render(UIPrimitive& primitive) override;
    };
}

#endif//_EOKAS_UI_IMAGE_H_
