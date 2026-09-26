#include "UIText.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    void UIText::render(UIPrimitive& primitive)
    {
        UIFont* font = UIFont::find(style.fontPath);
        if (!visible || font == nullptr || !font->isOpen())
        {
            return;
        }

        primitive.pushScaleAround(shape.origin, shape.scale);
        float scale = 1.0f;
        float ascender = 0.0f;
        float descender = 0.0f;
        font->drawMetrics(style.fontSize, scale, ascender, descender);
        float baseline = floorf(shape.top() + ascender + 0.5f);
        float cursorX = floorf(shape.left() + 0.5f);
        size_t index = 0;
        while (index < text.length())
        {
            uint32_t codepoint = 0;
            if (!UIFont::nextUtf8(text.cstr(), text.length(), index, codepoint))
            {
                continue;
            }
            const UIFontGlyph& g = font->glyphSized(codepoint, style.fontSize);
            if (g.uv.size.x > 0.0f && g.uv.size.y > 0.0f)
            {
                float destX = floorf(cursorX + g.bearingX * scale + 0.5f);
                float destY = floorf(baseline - g.bearingY * scale + 0.5f);
                float destW = (float)(int)(g.width * scale + 0.5f);
                float destH = (float)(int)(g.height * scale + 0.5f);
                Rect dest(destX, destY, destW, destH);
                primitive.addQuad(dest, g.uv, style.color);
            }
            cursorX += g.advance * scale;
        }

        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
