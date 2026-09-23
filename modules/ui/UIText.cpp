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
        float baseline = floorf(rect.y + font->ascender() * scale + 0.5f);
        float cursorX = floorf(rect.x + 0.5f);
        size_t index = 0;
        while (index < text.length())
        {
            uint32_t codepoint = 0;
            if (!UIFont::nextUtf8(text.cstr(), text.length(), index, codepoint))
            {
                continue;
            }
            const UIFontGlyph& g = font->glyph(codepoint);
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
