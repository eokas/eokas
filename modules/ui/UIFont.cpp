#include "UIFont.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <cmath>
#include <cstring>
#include <map>

namespace eokas
{
    UIFont::UIFont()
    {
        memset(mGlyphs, 0, sizeof(mGlyphs));
        memset(&mPlaceholder, 0, sizeof(mPlaceholder));
    }

    UIFont::~UIFont()
    {
        close();
    }

    bool UIFont::open(const char* fontPath, uint32_t pixelSize)
    {
        close();
        if (fontPath == nullptr || pixelSize == 0)
        {
            return false;
        }

        FT_Library library = nullptr;
        if (FT_Init_FreeType(&library) != 0)
        {
            return false;
        }

        FT_Face face = nullptr;
        if (FT_New_Face(library, fontPath, 0, &face) != 0)
        {
            FT_Done_FreeType(library);
            return false;
        }

        if (FT_Set_Pixel_Sizes(face, 0, pixelSize) != 0)
        {
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            return false;
        }
        FT_Select_Charmap(face, FT_ENCODING_UNICODE);

        mLibrary = library;
        mFace = face;
        mPixelSize = pixelSize;
        mAscender = (float)(face->size->metrics.ascender >> 6);
        mDescender = (float)(face->size->metrics.descender >> 6);
        mLineHeight = (float)(face->size->metrics.height >> 6);

        if (!bakeAtlas())
        {
            close();
            return false;
        }
        mAtlasDirty = true;
        return true;
    }

    bool UIFont::attachFallback(const char* fontPath)
    {
        if (mLibrary == nullptr || mFace == nullptr || fontPath == nullptr)
        {
            return false;
        }
        if (mFallback != nullptr)
        {
            FT_Done_Face((FT_Face)mFallback);
            mFallback = nullptr;
        }

        FT_Face face = nullptr;
        if (FT_New_Face((FT_Library)mLibrary, fontPath, 0, &face) != 0)
        {
            return false;
        }
        if (FT_Set_Pixel_Sizes(face, 0, mPixelSize) != 0)
        {
            FT_Done_Face(face);
            return false;
        }
        FT_Select_Charmap(face, FT_ENCODING_UNICODE);
        mFallback = face;
        return true;
    }

    void UIFont::close()
    {
        if (mFallback != nullptr)
        {
            FT_Done_Face((FT_Face)mFallback);
            mFallback = nullptr;
        }
        if (mFace != nullptr)
        {
            FT_Done_Face((FT_Face)mFace);
            mFace = nullptr;
        }
        if (mLibrary != nullptr)
        {
            FT_Done_FreeType((FT_Library)mLibrary);
            mLibrary = nullptr;
        }
        mPixelSize = 0;
        mAscender = 0.0f;
        mDescender = 0.0f;
        mLineHeight = 0.0f;
        mPenX = 0;
        mPenY = 0;
        mRowH = 0;
        mAtlas.clear();
        mDynamic.clear();
        mSizes.clear();
        mAtlasDirty = false;
        memset(mGlyphs, 0, sizeof(mGlyphs));
        memset(&mPlaceholder, 0, sizeof(mPlaceholder));
    }

    bool UIFont::isOpen() const
    {
        return mFace != nullptr && !mAtlas.empty();
    }

    uint32_t UIFont::atlasSize() const
    {
        return kAtlasSize;
    }

    const std::vector<uint8_t>& UIFont::atlasRgba() const
    {
        return mAtlas;
    }

    bool UIFont::takeAtlasDirty() const
    {
        bool dirty = mAtlasDirty;
        mAtlasDirty = false;
        return dirty;
    }

    uint32_t UIFont::pixelSize() const
    {
        return mPixelSize;
    }

    float UIFont::ascender() const
    {
        return mAscender;
    }

    float UIFont::descender() const
    {
        return mDescender;
    }

    float UIFont::lineHeight() const
    {
        return mLineHeight;
    }

    bool UIFont::setPixelSize(uint32_t pixelSize) const
    {
        if (mFace == nullptr || pixelSize == 0) return false;
        if (FT_Set_Pixel_Sizes((FT_Face)mFace, 0, pixelSize) != 0) return false;
        if (mFallback != nullptr && FT_Set_Pixel_Sizes((FT_Face)mFallback, 0, pixelSize) != 0) return false;
        return true;
    }

    bool UIFont::prepareSize(uint32_t pixelSize)
    {
        if (mFace == nullptr || pixelSize == 0) return false;
        if (pixelSize == mPixelSize || mSizes.find(pixelSize) != mSizes.end()) return true;
        if (!this->setPixelSize(pixelSize)) return false;

        SizeRun run;
        memset(run.glyphs, 0, sizeof(run.glyphs));
        FT_Face face = (FT_Face)mFace;
        run.ascender = (float)(face->size->metrics.ascender >> 6);
        run.descender = (float)(face->size->metrics.descender >> 6);
        run.lineHeight = (float)(face->size->metrics.height >> 6);
        for (uint32_t code = kFirstChar; code <= kLastChar; code++)
        {
            UIFontGlyph& slot = run.glyphs[code - kFirstChar];
            if (!this->packGlyph(code, false, slot)) slot = mPlaceholder;
        }
        mSizes.emplace(pixelSize, std::move(run));
        this->setPixelSize(mPixelSize);
        mAtlasDirty = true;
        return true;
    }

    bool UIFont::hasSize(uint32_t pixelSize) const
    {
        return pixelSize != 0 && (pixelSize == mPixelSize || mSizes.find(pixelSize) != mSizes.end());
    }

    float UIFont::ascenderFor(uint32_t pixelSize) const
    {
        if (pixelSize == mPixelSize) return mAscender;
        auto found = mSizes.find(pixelSize);
        if (found == mSizes.end()) return mAscender;
        return found->second.ascender;
    }

    float UIFont::descenderFor(uint32_t pixelSize) const
    {
        if (pixelSize == mPixelSize) return mDescender;
        auto found = mSizes.find(pixelSize);
        if (found == mSizes.end()) return mDescender;
        return found->second.descender;
    }

    void UIFont::drawMetrics(float fontSize, float& scale, float& ascender, float& descender) const
    {
        uint32_t px = (uint32_t)floorf(fontSize + 0.5f);
        if (px < 1) px = 1;
        if (this->hasSize(px))
        {
            scale = fontSize > 0.0f ? fontSize / (float)px : 1.0f;
            ascender = this->ascenderFor(px) * scale;
            descender = this->descenderFor(px) * scale;
            return;
        }
        float bake = mPixelSize > 0 ? (float)mPixelSize : 1.0f;
        scale = fontSize > 0.0f ? fontSize / bake : 1.0f;
        ascender = mAscender * scale;
        descender = mDescender * scale;
    }

    const UIFontGlyph& UIFont::glyphFor(uint32_t codepoint, uint32_t pixelSize) const
    {
        auto found = mSizes.find(pixelSize);
        if (found == mSizes.end()) return this->glyph(codepoint);
        SizeRun& run = found->second;
        if (codepoint >= kFirstChar && codepoint <= kLastChar) return run.glyphs[codepoint - kFirstChar];
        auto dynamic = run.dynamic.find(codepoint);
        if (dynamic != run.dynamic.end()) return dynamic->second;
        if (!this->setPixelSize(pixelSize)) return mPlaceholder;
        UIFontGlyph baked;
        bool packed = this->packGlyph(codepoint, false, baked);
        this->setPixelSize(mPixelSize);
        if (!packed) return mPlaceholder;
        mAtlasDirty = true;
        return run.dynamic.emplace(codepoint, baked).first->second;
    }

    const UIFontGlyph& UIFont::glyphSized(uint32_t codepoint, float fontSize) const
    {
        uint32_t px = (uint32_t)floorf(fontSize + 0.5f);
        if (px >= 1 && px != mPixelSize && this->hasSize(px)) return this->glyphFor(codepoint, px);
        return this->glyph(codepoint);
    }

    const UIFontGlyph& UIFont::glyph(char c) const
    {
        return this->glyph((uint32_t)(unsigned char)c);
    }

    const UIFontGlyph& UIFont::glyph(uint32_t codepoint) const
    {
        if (codepoint >= kFirstChar && codepoint <= kLastChar)
        {
            return mGlyphs[codepoint - kFirstChar];
        }
        auto found = mDynamic.find(codepoint);
        if (found != mDynamic.end())
        {
            return found->second;
        }
        UIFontGlyph baked;
        if (!this->packGlyph(codepoint, false, baked))
        {
            return mPlaceholder;
        }
        mAtlasDirty = true;
        return mDynamic.emplace(codepoint, baked).first->second;
    }

    bool UIFont::nextUtf8(const char* data, size_t size, size_t& index, uint32_t& codepoint)
    {
        if (data == nullptr || index >= size)
        {
            return false;
        }
        const unsigned char* bytes = (const unsigned char*)data;
        unsigned char lead = bytes[index];
        if (lead < 0x80)
        {
            codepoint = lead;
            index += 1;
            return true;
        }
        if ((lead & 0xE0) == 0xC0 && index + 1 < size && (bytes[index + 1] & 0xC0) == 0x80)
        {
            codepoint = ((uint32_t)(lead & 0x1F) << 6) | (uint32_t)(bytes[index + 1] & 0x3F);
            index += 2;
            return codepoint >= 0x80;
        }
        if ((lead & 0xF0) == 0xE0 && index + 2 < size
            && (bytes[index + 1] & 0xC0) == 0x80
            && (bytes[index + 2] & 0xC0) == 0x80)
        {
            codepoint = ((uint32_t)(lead & 0x0F) << 12)
                | ((uint32_t)(bytes[index + 1] & 0x3F) << 6)
                | (uint32_t)(bytes[index + 2] & 0x3F);
            index += 3;
            return codepoint >= 0x800;
        }
        if ((lead & 0xF8) == 0xF0 && index + 3 < size
            && (bytes[index + 1] & 0xC0) == 0x80
            && (bytes[index + 2] & 0xC0) == 0x80
            && (bytes[index + 3] & 0xC0) == 0x80)
        {
            codepoint = ((uint32_t)(lead & 0x07) << 18)
                | ((uint32_t)(bytes[index + 1] & 0x3F) << 12)
                | ((uint32_t)(bytes[index + 2] & 0x3F) << 6)
                | (uint32_t)(bytes[index + 3] & 0x3F);
            index += 4;
            return codepoint >= 0x10000 && codepoint <= 0x10FFFF;
        }
        index += 1;
        codepoint = 0;
        return false;
    }

    String UIFont::encodeUtf8(uint32_t codepoint)
    {
        char buf[5] = {};
        size_t n = 0;
        if (codepoint < 0x80)
        {
            buf[0] = (char)codepoint;
            n = 1;
        }
        else if (codepoint < 0x800)
        {
            buf[0] = (char)(0xC0 | (codepoint >> 6));
            buf[1] = (char)(0x80 | (codepoint & 0x3F));
            n = 2;
        }
        else if (codepoint < 0x10000)
        {
            buf[0] = (char)(0xE0 | (codepoint >> 12));
            buf[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            buf[2] = (char)(0x80 | (codepoint & 0x3F));
            n = 3;
        }
        else
        {
            buf[0] = (char)(0xF0 | (codepoint >> 18));
            buf[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
            buf[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            buf[3] = (char)(0x80 | (codepoint & 0x3F));
            n = 4;
        }
        return String(buf, n);
    }

    Rect UIFont::solidUV()
    {
        float inv = 1.0f / (float)kAtlasSize;
        return Rect(1.0f * inv, 1.0f * inv, 2.0f * inv, 2.0f * inv);
    }

    bool UIFont::bakeAtlas()
    {
        const uint32_t size = kAtlasSize;
        mAtlas.assign(size * size * 4, 0);
        memset(mGlyphs, 0, sizeof(mGlyphs));
        memset(&mPlaceholder, 0, sizeof(mPlaceholder));

        for (uint32_t y = 0; y < 4; y++)
        {
            for (uint32_t x = 0; x < 4; x++)
            {
                uint32_t i = (y * size + x) * 4;
                mAtlas[i + 0] = 0xFF;
                mAtlas[i + 1] = 0xFF;
                mAtlas[i + 2] = 0xFF;
                mAtlas[i + 3] = 0xFF;
            }
        }

        mPenX = 6;
        mPenY = 0;
        mRowH = 4;

        if (!packGlyph(0, true, mPlaceholder))
        {
            if (!packGlyph((uint32_t)'?', false, mPlaceholder))
            {
                return false;
            }
        }

        for (uint32_t code = kFirstChar; code <= kLastChar; code++)
        {
            UIFontGlyph& slot = mGlyphs[code - kFirstChar];
            if (!packGlyph(code, false, slot))
            {
                slot = mPlaceholder;
            }
        }
        return true;
    }

    bool UIFont::packGlyph(uint32_t codeOrZero, bool useGlyphIndex, UIFontGlyph& out) const
    {
        FT_Face face = (FT_Face)mFace;
        if (!useGlyphIndex && mFallback != nullptr && FT_Get_Char_Index(face, codeOrZero) == 0)
        {
            FT_Face fallback = (FT_Face)mFallback;
            if (FT_Get_Char_Index(fallback, codeOrZero) != 0)
            {
                face = fallback;
            }
        }
        const FT_Int32 loadFlags = FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_NORMAL;
        FT_Error err = useGlyphIndex
            ? FT_Load_Glyph(face, codeOrZero, loadFlags)
            : FT_Load_Char(face, codeOrZero, loadFlags);
        if (err != 0)
        {
            return false;
        }

        FT_GlyphSlot slot = face->glyph;
        if (slot->format != FT_GLYPH_FORMAT_BITMAP)
        {
            if (FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL) != 0)
            {
                return false;
            }
        }
        uint32_t gw = slot->bitmap.width;
        uint32_t gh = slot->bitmap.rows;
        const unsigned char* coverage = slot->bitmap.buffer;
        int coveragePitch = slot->bitmap.pitch;
        std::vector<unsigned char> gray;
        if (gh > 0 && gw > 0 && coverage != nullptr && slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
        {
            if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
            {
                gray.assign((size_t)gw * gh, 0);
                for (uint32_t y = 0; y < gh; y++)
                {
                    const unsigned char* row = coverage + (int)y * coveragePitch;
                    for (uint32_t x = 0; x < gw; x++)
                    {
                        unsigned char bit = (unsigned char)((row[x >> 3] >> (7 - (x & 7))) & 1);
                        gray[(size_t)y * gw + x] = bit ? 255 : 0;
                    }
                }
            }
            else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD)
            {
                uint32_t pixels = gw / 3;
                gray.assign((size_t)pixels * gh, 0);
                for (uint32_t y = 0; y < gh; y++)
                {
                    const unsigned char* row = coverage + (int)y * coveragePitch;
                    for (uint32_t x = 0; x < pixels; x++)
                    {
                        unsigned int sum = (unsigned int)row[x * 3] + row[x * 3 + 1] + row[x * 3 + 2];
                        gray[(size_t)y * pixels + x] = (unsigned char)(sum / 3);
                    }
                }
                gw = pixels;
            }
            else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V)
            {
                uint32_t rows = gh / 3;
                gray.assign((size_t)gw * rows, 0);
                for (uint32_t y = 0; y < rows; y++)
                {
                    const unsigned char* rowR = coverage + (int)(y * 3) * coveragePitch;
                    const unsigned char* rowG = coverage + (int)(y * 3 + 1) * coveragePitch;
                    const unsigned char* rowB = coverage + (int)(y * 3 + 2) * coveragePitch;
                    for (uint32_t x = 0; x < gw; x++)
                    {
                        gray[(size_t)y * gw + x] = (unsigned char)(((unsigned int)rowR[x] + rowG[x] + rowB[x]) / 3);
                    }
                }
                gh = rows;
            }
            else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA)
            {
                gray.assign((size_t)gw * gh, 0);
                for (uint32_t y = 0; y < gh; y++)
                {
                    const unsigned char* row = coverage + (int)y * coveragePitch;
                    for (uint32_t x = 0; x < gw; x++)
                    {
                        gray[(size_t)y * gw + x] = row[x * 4 + 3];
                    }
                }
            }
            else
            {
                return false;
            }
            coverage = gray.data();
            coveragePitch = (int)gw;
        }
        const uint32_t padding = 2;

        if (gw > 0 && gh > 0)
        {
            if (mPenX + gw + padding > kAtlasSize)
            {
                mPenX = 0;
                mPenY += mRowH + padding;
                mRowH = 0;
            }
            if (mPenY + gh + padding > kAtlasSize)
            {
                return false;
            }
            if (coverage != nullptr)
            {
                blitGlyph(coverage, coveragePitch, gw, gh, mPenX, mPenY);
            }

            float inv = 1.0f / (float)kAtlasSize;
            out.uv = Rect((float)mPenX * inv, (float)mPenY * inv, (float)gw * inv, (float)gh * inv);
            mPenX += gw + padding;
            if (gh > mRowH)
            {
                mRowH = gh;
            }
        }
        else
        {
            out.uv = Rect(0.0f, 0.0f, 0.0f, 0.0f);
        }

        out.width = (float)gw;
        out.height = (float)gh;
        out.bearingX = (float)slot->bitmap_left;
        out.bearingY = (float)slot->bitmap_top;
        out.advance = (float)(slot->advance.x >> 6);
        return true;
    }

    void UIFont::blitGlyph(const unsigned char* src, int pitch, uint32_t srcW, uint32_t srcH, uint32_t dstX, uint32_t dstY) const
    {
        if (src == nullptr || pitch == 0)
        {
            return;
        }

        for (uint32_t y = 0; y < srcH; y++)
        {
            const unsigned char* row = pitch >= 0
                ? src + (int)y * pitch
                : src + (int)(srcH - 1 - y) * (-pitch);
            for (uint32_t x = 0; x < srcW; x++)
            {
                uint32_t i = ((dstY + y) * kAtlasSize + (dstX + x)) * 4;
                uint8_t a = row[x];
                mAtlas[i + 0] = 0xFF;
                mAtlas[i + 1] = 0xFF;
                mAtlas[i + 2] = 0xFF;
                mAtlas[i + 3] = a;
            }
        }
    }
}
