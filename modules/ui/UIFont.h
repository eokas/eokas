#ifndef _EOKAS_UI_FONT_H_
#define _EOKAS_UI_FONT_H_

#include "header.h"
#include <map>
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
        static constexpr uint32_t kAtlasSize = 1024;
        static constexpr uint32_t kFirstChar = 32;
        static constexpr uint32_t kLastChar = 126;
        static constexpr uint32_t kGlyphCount = kLastChar - kFirstChar + 1;

        UIFont();
        ~UIFont();
        UIFont(const UIFont&) = delete;
        UIFont& operator=(const UIFont&) = delete;

        bool open(const char* fontPath, uint32_t pixelSize);
        bool attachFallback(const char* fontPath);
        void close();
        bool isOpen() const;

        uint32_t atlasSize() const;
        const std::vector<uint8_t>& atlasRgba() const;
        bool takeAtlasDirty() const;
        uint32_t pixelSize() const;
        float ascender() const;
        float descender() const;
        float lineHeight() const;
        bool prepareSize(uint32_t pixelSize);
        bool hasSize(uint32_t pixelSize) const;
        void drawMetrics(float fontSize, float& scale, float& ascender, float& descender) const;
        const UIFontGlyph& glyph(uint32_t codepoint) const;
        const UIFontGlyph& glyph(char c) const;
        const UIFontGlyph& glyphSized(uint32_t codepoint, float fontSize) const;
        static bool nextUtf8(const char* data, size_t size, size_t& index, uint32_t& codepoint);
        static String encodeUtf8(uint32_t codepoint);
        static Rect solidUV();

    private:
        struct SizeRun
        {
            float ascender = 0.0f;
            float descender = 0.0f;
            float lineHeight = 0.0f;
            UIFontGlyph glyphs[kGlyphCount];
            std::map<uint32_t, UIFontGlyph> dynamic;
        };

        bool bakeAtlas();
        bool setPixelSize(uint32_t pixelSize) const;
        bool packGlyph(uint32_t codeOrZero, bool useGlyphIndex, UIFontGlyph& out) const;
        void blitGlyph(const unsigned char* src, int pitch, uint32_t srcW, uint32_t srcH, uint32_t dstX, uint32_t dstY) const;
        const UIFontGlyph& glyphFor(uint32_t codepoint, uint32_t pixelSize) const;
        float ascenderFor(uint32_t pixelSize) const;
        float descenderFor(uint32_t pixelSize) const;

        void* mLibrary = nullptr;
        void* mFace = nullptr;
        void* mFallback = nullptr;
        uint32_t mPixelSize = 0;
        float mAscender = 0.0f;
        float mDescender = 0.0f;
        float mLineHeight = 0.0f;
        mutable uint32_t mPenX = 0;
        mutable uint32_t mPenY = 0;
        mutable uint32_t mRowH = 0;
        mutable bool mAtlasDirty = false;
        mutable std::vector<uint8_t> mAtlas;
        mutable std::map<uint32_t, UIFontGlyph> mDynamic;
        mutable std::map<uint32_t, SizeRun> mSizes;
        UIFontGlyph mGlyphs[kGlyphCount];
        UIFontGlyph mPlaceholder;
    };
}

#endif//_EOKAS_UI_FONT_H_
