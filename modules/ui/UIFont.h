#ifndef _EOKAS_UI_FONT_H_
#define _EOKAS_UI_FONT_H_

#include "header.h"
#include <vector>

namespace eokas
{
    struct UIFontGlyph
    {
        Rect uv;
        float width = 0.0f;
        float height = 0.0f;
        float bearingX = 0.0f;
        float bearingY = 0.0f;
        float advance = 0.0f;
    };

    class UIFont
    {
    public:
        static constexpr uint32_t kAtlasSize = 512;
        static constexpr uint32_t kFirstChar = 32;
        static constexpr uint32_t kLastChar = 126;
        static constexpr uint32_t kGlyphCount = kLastChar - kFirstChar + 1;

        UIFont();
        ~UIFont();
        UIFont(const UIFont&) = delete;
        UIFont& operator=(const UIFont&) = delete;

        bool open(const char* fontPath, uint32_t pixelSize);
        void close();
        bool isOpen() const;

        uint32_t atlasSize() const;
        const std::vector<uint8_t>& atlasRgba() const;
        uint32_t pixelSize() const;
        float ascender() const;
        float lineHeight() const;
        const UIFontGlyph& glyph(char c) const;
        static Rect solidUV();

    private:
        bool bakeAtlas();
        bool packGlyph(uint32_t codeOrZero, bool useGlyphIndex, UIFontGlyph& out);
        void blitGlyph(const unsigned char* src, int pitch, uint32_t srcW, uint32_t srcH, uint32_t dstX, uint32_t dstY);

        void* mLibrary = nullptr;
        void* mFace = nullptr;
        uint32_t mPixelSize = 0;
        float mAscender = 0.0f;
        float mLineHeight = 0.0f;
        uint32_t mPenX = 0;
        uint32_t mPenY = 0;
        uint32_t mRowH = 0;
        std::vector<uint8_t> mAtlas;
        UIFontGlyph mGlyphs[kGlyphCount];
        UIFontGlyph mPlaceholder;
    };
}

#endif//_EOKAS_UI_FONT_H_
