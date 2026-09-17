#include "UIText.h"
#include "UIFont.h"
#include <cmath>

namespace eokas
{
    void UIText::render(UIShape& shape)
    {
        if (!visible || font == nullptr || !font->isOpen())
        {
            return;
        }

        float bake = (float)font->pixelSize();
        float scale = (fontSize > 0.0f ? fontSize : bake) / bake;
        float baseline = rect.y + font->ascender() * scale;
        float cursorX = rect.x;
        for (size_t i = 0; i < text.length(); i++)
        {
            const UIFontGlyph& g = font->glyph(text.at(i));
            if (g.uv.width > 0.0f && g.uv.height > 0.0f)
            {
                float destX = floorf(cursorX + g.bearingX * scale + 0.5f);
                float destY = floorf(baseline - g.bearingY * scale + 0.5f);
                float destW = (float)(int)(g.width * scale + 0.5f);
                float destH = (float)(int)(g.height * scale + 0.5f);
                Rect dest(destX, destY, destW, destH);
                shape.addQuad(dest, g.uv, color);
            }
            cursorX += g.advance * scale;
        }

        UIWidget::render(shape);
    }
}
