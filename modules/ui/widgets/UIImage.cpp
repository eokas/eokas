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

        bool buildSliceGrid(const UIImage& image, const UIShape& shape, SliceGrid& grid)
        {
            float atlasW = 0.0f;
            float atlasH = 0.0f;
            if (shape.texture)
            {
                const TextureOptions& options = shape.texture->getOptions();
                atlasW = (float)options.width;
                atlasH = (float)options.height;
            }
            float spriteW = image.uv.width * atlasW;
            float spriteH = image.uv.height * atlasH;
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
            clampPair(destL, destR, image.rect.width);
            clampPair(destT, destB, image.rect.height);

            grid.x[0] = image.rect.x;
            grid.x[1] = image.rect.x + destL;
            grid.x[2] = image.rect.x + image.rect.width - destR;
            grid.x[3] = image.rect.x + image.rect.width;
            grid.y[0] = image.rect.y;
            grid.y[1] = image.rect.y + destT;
            grid.y[2] = image.rect.y + image.rect.height - destB;
            grid.y[3] = image.rect.y + image.rect.height;

            float uL = srcL / atlasW;
            float uR = srcR / atlasW;
            float vT = srcT / atlasH;
            float vB = srcB / atlasH;
            grid.u[0] = image.uv.x;
            grid.u[1] = image.uv.x + uL;
            grid.u[2] = image.uv.x + image.uv.width - uR;
            grid.u[3] = image.uv.x + image.uv.width;
            grid.v[0] = image.uv.y;
            grid.v[1] = image.uv.y + vT;
            grid.v[2] = image.uv.y + image.uv.height - vB;
            grid.v[3] = image.uv.y + image.uv.height;
            grid.halfU = 0.5f / atlasW;
            grid.halfV = 0.5f / atlasH;
            return true;
        }

        void addCell(UIShape& shape, const Color& color,
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
            shape.addQuad(Rect(x0, y0, w, h), Rect(u0, v0, uw, vh), color);
        }

        void renderSliced(UIImage& image, UIShape& shape, const SliceGrid& grid)
        {
            for (int row = 0; row < 3; row++)
            {
                for (int col = 0; col < 3; col++)
                {
                    if (!image.fillCenter && row == 1 && col == 1)
                    {
                        continue;
                    }
                    addCell(shape, image.color,
                        grid.x[col], grid.y[row], grid.x[col + 1], grid.y[row + 1],
                        grid.u[col], grid.v[row], grid.u[col + 1], grid.v[row + 1],
                        grid.halfU, grid.halfV);
                }
            }
        }
    }

    void UIImage::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        SliceGrid grid;
        if (type == UIImageType::Simple || !buildSliceGrid(*this, shape, grid))
        {
            shape.addQuad(rect, uv, color);
        }
        else
        {
            renderSliced(*this, shape, grid);
        }
        UIWidget::render(shape);
    }
}
