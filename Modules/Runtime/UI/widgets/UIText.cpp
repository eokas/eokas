#include "UIText.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    void UIText::layout()
    {
        if (!visible)
        {
            return;
        }
        UIFont* font = UIFont::find(style.fontPath);
        if (font == nullptr || !font->isOpen())
        {
            return;
        }
        mShown = text;
        Vector2 measured = font->measure(text, style.fontSize);
        float width = measured.x;
        float height = measured.y;
        if (maxWidth > 0.0f && width > maxWidth)
        {
            mShown = font->cut(text, style.fontSize, maxWidth);
            width = font->measure(mShown, style.fontSize).x;
            if (width > maxWidth)
            {
                width = maxWidth;
            }
        }
        this->resize(Vector2(floorf(width + 0.5f), floorf(height + 0.5f)));
    }

    void UIText::render(UIPrimitive& primitive)
    {
        UIFont* font = UIFont::find(style.fontPath);
        if (!visible || font == nullptr || !font->isOpen())
        {
            return;
        }

        Matrix3 localToScreen = matrixLocalToScreen();
        float scale = 1.0f;
        float ascender = 0.0f;
        float descender = 0.0f;
        font->drawMetrics(style.fontSize, scale, ascender, descender);
        float baseline = floorf(ascender + 0.5f);
        float cursorX = floorf(0.5f);
        const String& shown = mShown.isEmpty() && !text.isEmpty() ? text : mShown;
        size_t index = 0;
        while (index < shown.length())
        {
            uint32_t codepoint = 0;
            if (!UIFont::nextUtf8(shown.cstr(), shown.length(), index, codepoint))
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
                primitive.addQuad(localToScreen, dest, g.uv, style.color);
            }
            cursorX += g.advance * scale;
        }

        UIWidget::render(primitive);
    }
}
