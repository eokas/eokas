#include "UIImage.h"

namespace eokas
{
    namespace
    {
        struct SliceGrid
        {
            float x[4];
            float y[4];
            float u[4];
            float v[4];
            float halfU = 0.0f;
            float halfV = 0.0f;
        };

        void clampPair(float& a, float& b, float limit)
        {
            if (a < 0.0f) a = 0.0f;
            if (b < 0.0f) b = 0.0f;
            float sum = a + b;
            if (sum > 0.0f && sum > limit)
            {
                float scale = limit / sum;
                a *= scale;
                b *= scale;
            }
        }

        bool buildSliceGrid(const UIImage& image, const UIPrimitive& primitive, SliceGrid& grid)
        {
            float atlasW = 0.0f;
            float atlasH = 0.0f;
            if (primitive.texture)
            {
                const TextureOptions& options = primitive.texture->getOptions();
                atlasW = (float)options.width;
                atlasH = (float)options.height;
            }
            float spriteW = image.uv.size.x * atlasW;
            float spriteH = image.uv.size.y * atlasH;
            if (spriteW <= 0.0f || spriteH <= 0.0f)
            {
                return false;
            }

            float srcL = image.border.left;
            float srcT = image.border.top;
            float srcR = image.border.right;
            float srcB = image.border.bottom;
            clampPair(srcL, srcR, spriteW);
            clampPair(srcT, srcB, spriteH);

            float destL = srcL;
            float destT = srcT;
            float destR = srcR;
            float destB = srcB;
            clampPair(destL, destR, image.rect.size.x);
            clampPair(destT, destB, image.rect.size.y);

            grid.x[0] = image.rect.origin.x;
            grid.x[1] = image.rect.origin.x + destL;
            grid.x[2] = image.rect.origin.x + image.rect.size.x - destR;
            grid.x[3] = image.rect.origin.x + image.rect.size.x;
            grid.y[0] = image.rect.origin.y;
            grid.y[1] = image.rect.origin.y + destT;
            grid.y[2] = image.rect.origin.y + image.rect.size.y - destB;
            grid.y[3] = image.rect.origin.y + image.rect.size.y;

            float uL = srcL / atlasW;
            float uR = srcR / atlasW;
            float vT = srcT / atlasH;
            float vB = srcB / atlasH;
            grid.u[0] = image.uv.origin.x;
            grid.u[1] = image.uv.origin.x + uL;
            grid.u[2] = image.uv.origin.x + image.uv.size.x - uR;
            grid.u[3] = image.uv.origin.x + image.uv.size.x;
            grid.v[0] = image.uv.origin.y;
            grid.v[1] = image.uv.origin.y + vT;
            grid.v[2] = image.uv.origin.y + image.uv.size.y - vB;
            grid.v[3] = image.uv.origin.y + image.uv.size.y;
            grid.halfU = 0.5f / atlasW;
            grid.halfV = 0.5f / atlasH;
            return true;
        }

        void addCell(UIPrimitive& primitive, const Color& color,
            float x0, float y0, float x1, float y1,
            float u0, float v0, float u1, float v1,
            float halfU, float halfV)
        {
            float w = x1 - x0;
            float h = y1 - y0;
            float uw = u1 - u0;
            float vh = v1 - v0;
            if (w <= 0.0f || h <= 0.0f || uw <= 0.0f || vh <= 0.0f)
            {
                return;
            }
            if (uw > halfU * 2.0f)
            {
                u0 += halfU;
                u1 -= halfU;
                uw = u1 - u0;
            }
            if (vh > halfV * 2.0f)
            {
                v0 += halfV;
                v1 -= halfV;
                vh = v1 - v0;
            }
            primitive.addQuad(Rect(x0, y0, w, h), Rect(u0, v0, uw, vh), color);
        }

        void renderSliced(UIImage& image, UIPrimitive& primitive, const SliceGrid& grid)
        {
            for (int row = 0; row < 3; row++)
            {
                for (int col = 0; col < 3; col++)
                {
                    if (!image.fillCenter && row == 1 && col == 1)
                    {
                        continue;
                    }
                    addCell(primitive, image.fill,
                        grid.x[col], grid.y[row], grid.x[col + 1], grid.y[row + 1],
                        grid.u[col], grid.v[row], grid.u[col + 1], grid.v[row + 1],
                        grid.halfU, grid.halfV);
                }
            }
        }
    }

    void UIImage::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        SliceGrid grid;
        if (type == UIImageType::Simple || !buildSliceGrid(*this, primitive, grid))
        {
            primitive.addQuad(rect, uv, fill);
        }
        else
        {
            renderSliced(*this, primitive, grid);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
