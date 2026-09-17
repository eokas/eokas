#include "UIFont.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <cstring>

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

        mLibrary = library;
        mFace = face;
        mPixelSize = pixelSize;
        mAscender = (float)(face->size->metrics.ascender >> 6);
        mLineHeight = (float)(face->size->metrics.height >> 6);

        if (!bakeAtlas())
        {
            close();
            return false;
        }
        return true;
    }

    void UIFont::close()
    {
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
        mLineHeight = 0.0f;
        mPenX = 0;
        mPenY = 0;
        mRowH = 0;
        mAtlas.clear();
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

    uint32_t UIFont::pixelSize() const
    {
        return mPixelSize;
    }

    float UIFont::ascender() const
    {
        return mAscender;
    }

    float UIFont::lineHeight() const
    {
        return mLineHeight;
    }

    const UIFontGlyph& UIFont::glyph(char c) const
    {
        unsigned char uc = (unsigned char)c;
        if (uc >= kFirstChar && uc <= kLastChar)
        {
            return mGlyphs[uc - kFirstChar];
        }
        return mPlaceholder;
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

    bool UIFont::packGlyph(uint32_t codeOrZero, bool useGlyphIndex, UIFontGlyph& out)
    {
        FT_Face face = (FT_Face)mFace;
        const FT_Int32 loadFlags = FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT;
        FT_Error err = useGlyphIndex
            ? FT_Load_Glyph(face, codeOrZero, loadFlags)
            : FT_Load_Char(face, codeOrZero, loadFlags);
        if (err != 0)
        {
            return false;
        }

        FT_GlyphSlot slot = face->glyph;
        uint32_t gw = slot->bitmap.width;
        uint32_t gh = slot->bitmap.rows;
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
            if (slot->bitmap.buffer != nullptr)
            {
                blitGlyph(slot->bitmap.buffer, slot->bitmap.pitch, gw, gh, mPenX, mPenY);
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

    void UIFont::blitGlyph(const unsigned char* src, int pitch, uint32_t srcW, uint32_t srcH, uint32_t dstX, uint32_t dstY)
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
