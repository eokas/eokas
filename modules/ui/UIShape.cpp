#include "UIShape.h"
#include <cstring>
#include <stdexcept>
#include <vector>

namespace eokas
{
    namespace
    {
        struct ClipVert
        {
            float x;
            float y;
            float u;
            float v;
        };

        float lesser(float a, float b)
        {
            return a < b ? a : b;
        }

        float greater(float a, float b)
        {
            return a > b ? a : b;
        }

        ClipVert intersectAtX(const ClipVert& a, const ClipVert& b, float x)
        {
            float dx = b.x - a.x;
            float t = (dx != 0.0f) ? (x - a.x) / dx : 0.0f;
            ClipVert out;
            out.x = x;
            out.y = a.y + (b.y - a.y) * t;
            out.u = a.u + (b.u - a.u) * t;
            out.v = a.v + (b.v - a.v) * t;
            return out;
        }

        ClipVert intersectAtY(const ClipVert& a, const ClipVert& b, float y)
        {
            float dy = b.y - a.y;
            float t = (dy != 0.0f) ? (y - a.y) / dy : 0.0f;
            ClipVert out;
            out.x = a.x + (b.x - a.x) * t;
            out.y = y;
            out.u = a.u + (b.u - a.u) * t;
            out.v = a.v + (b.v - a.v) * t;
            return out;
        }

        void clipEdge(const std::vector<ClipVert>& input, std::vector<ClipVert>& output, const Rect& clip, int edge)
        {
            output.clear();
            if (input.empty())
            {
                return;
            }
            float limit = clip.x;
            if (edge == 1)
            {
                limit = clip.x + clip.width;
            }
            else if (edge == 2)
            {
                limit = clip.y;
            }
            else if (edge == 3)
            {
                limit = clip.y + clip.height;
            }
            for (size_t i = 0; i < input.size(); ++i)
            {
                const ClipVert& current = input[i];
                const ClipVert& previous = input[(i + input.size() - 1) % input.size()];
                bool currentInside = false;
                bool previousInside = false;
                ClipVert hit;
                if (edge == 0)
                {
                    currentInside = current.x >= limit;
                    previousInside = previous.x >= limit;
                    hit = intersectAtX(previous, current, limit);
                }
                else if (edge == 1)
                {
                    currentInside = current.x <= limit;
                    previousInside = previous.x <= limit;
                    hit = intersectAtX(previous, current, limit);
                }
                else if (edge == 2)
                {
                    currentInside = current.y >= limit;
                    previousInside = previous.y >= limit;
                    hit = intersectAtY(previous, current, limit);
                }
                else
                {
                    currentInside = current.y <= limit;
                    previousInside = previous.y <= limit;
                    hit = intersectAtY(previous, current, limit);
                }
                if (currentInside)
                {
                    if (!previousInside)
                    {
                        output.push_back(hit);
                    }
                    output.push_back(current);
                }
                else if (previousInside)
                {
                    output.push_back(hit);
                }
            }
        }

        void emitTriangle(std::vector<UIVertex>& vertices, std::vector<uint32_t>& indices, const ClipVert& a, const ClipVert& b, const ClipVert& c, const Vector4& color)
        {
            uint32_t base = (uint32_t)vertices.size();
            vertices.push_back({ Vector2(a.x, a.y), Vector2(a.u, a.v), color });
            vertices.push_back({ Vector2(b.x, b.y), Vector2(b.u, b.v), color });
            vertices.push_back({ Vector2(c.x, c.y), Vector2(c.u, c.v), color });
            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
        }

        void emitClippedTriangle(std::vector<UIVertex>& vertices, std::vector<uint32_t>& indices, const ClipVert& a, const ClipVert& b, const ClipVert& c, const Rect& clip, const Vector4& color)
        {
            if (clip.width <= 0.0f || clip.height <= 0.0f)
            {
                return;
            }
            std::vector<ClipVert> poly;
            poly.push_back(a);
            poly.push_back(b);
            poly.push_back(c);
            std::vector<ClipVert> next;
            for (int edge = 0; edge < 4; ++edge)
            {
                clipEdge(poly, next, clip, edge);
                poly.swap(next);
            }
            if (poly.size() < 3)
            {
                return;
            }
            for (size_t i = 1; i + 1 < poly.size(); ++i)
            {
                emitTriangle(vertices, indices, poly[0], poly[i], poly[i + 1], color);
            }
        }
    }

    UIShape::UIShape()
    {
        screenSpace = true;
    }

    void UIShape::begin()
    {
        mVertices.clear();
        mIndices.clear();
        indexCount = 0;
        mOffset = Vector2(0.0f, 0.0f);
        mOffsetStack.clear();
        mClipStack.clear();
    }

    void UIShape::pushOffset(const Vector2& next)
    {
        mOffsetStack.push_back(mOffset);
        mOffset = next;
    }

    void UIShape::popOffset()
    {
        if (mOffsetStack.empty())
        {
            return;
        }
        mOffset = mOffsetStack.back();
        mOffsetStack.pop_back();
    }

    void UIShape::pushClip(const Rect& screenClip)
    {
        Rect next = screenClip;
        if (!mClipStack.empty())
        {
            const Rect& prev = mClipStack.back();
            float x0 = greater(prev.x, screenClip.x);
            float y0 = greater(prev.y, screenClip.y);
            float x1 = lesser(prev.x + prev.width, screenClip.x + screenClip.width);
            float y1 = lesser(prev.y + prev.height, screenClip.y + screenClip.height);
            float w = x1 - x0;
            float h = y1 - y0;
            if (w < 0.0f)
            {
                w = 0.0f;
            }
            if (h < 0.0f)
            {
                h = 0.0f;
            }
            next = Rect(x0, y0, w, h);
        }
        mClipStack.push_back(next);
    }

    void UIShape::popClip()
    {
        if (!mClipStack.empty())
        {
            mClipStack.pop_back();
        }
    }

    bool UIShape::outsideClip(const Rect& local) const
    {
        if (mClipStack.empty())
        {
            return false;
        }
        const Rect& clip = mClipStack.back();
        if (clip.width <= 0.0f || clip.height <= 0.0f)
        {
            return true;
        }
        float x0 = local.x + mOffset.x;
        float y0 = local.y + mOffset.y;
        float x1 = x0 + local.width;
        float y1 = y0 + local.height;
        float cx1 = clip.x + clip.width;
        float cy1 = clip.y + clip.height;
        return x1 < clip.x || x0 > cx1 || y1 < clip.y || y0 > cy1;
    }

    void UIShape::setTexture(Texture::Ref tex)
    {
        texture = tex;
    }

    void UIShape::setPendingUpload(const std::vector<uint8_t>& rgba, uint32_t atlasSize)
    {
        mPendingUploadRgba = rgba;
        mPendingAtlasSize = atlasSize;
        mTextureDirty = true;
    }

    void UIShape::addQuad(const Rect& screen, const Rect& uv, const Color& color)
    {
        this->addQuad(
            Vector2(screen.x, screen.y),
            Vector2(screen.x + screen.width, screen.y),
            Vector2(screen.x + screen.width, screen.y + screen.height),
            Vector2(screen.x, screen.y + screen.height),
            uv,
            color);
    }

    void UIShape::addQuad(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, const Rect& uv, const Color& color)
    {
        Vector4 vertexColor(color.r, color.g, color.b, color.a);
        ClipVert q0 { p0.x + mOffset.x, p0.y + mOffset.y, uv.x, uv.y };
        ClipVert q1 { p1.x + mOffset.x, p1.y + mOffset.y, uv.x + uv.width, uv.y };
        ClipVert q2 { p2.x + mOffset.x, p2.y + mOffset.y, uv.x + uv.width, uv.y + uv.height };
        ClipVert q3 { p3.x + mOffset.x, p3.y + mOffset.y, uv.x, uv.y + uv.height };

        if (mClipStack.empty())
        {
            emitTriangle(mVertices, mIndices, q0, q1, q2, vertexColor);
            emitTriangle(mVertices, mIndices, q0, q2, q3, vertexColor);
            return;
        }

        const Rect& clip = mClipStack.back();
        emitClippedTriangle(mVertices, mIndices, q0, q1, q2, clip, vertexColor);
        emitClippedTriangle(mVertices, mIndices, q0, q2, q3, clip, vertexColor);
    }

    void UIShape::end()
    {
        if (mVertices.size() > mMaxQuads * 4)
        {
            throw std::runtime_error("UIShape: vertex count exceeds maxQuads.");
        }

        indexCount = (uint32_t)mIndices.size();
        vertexLength = (uint32_t)(mVertices.size() * sizeof(UIVertex));
        indexLength = (uint32_t)(mIndices.size() * sizeof(uint32_t));
    }

    void UIShape::createResources(Device::Ref device)
    {
        vertexStride = sizeof(UIVertex);
        vertexLength = mMaxQuads * 4 * vertexStride;
        indexFormat = Format::R32_UINT;
        indexLength = mMaxQuads * 6 * sizeof(uint32_t);
        if (!vertexBuffer)
            vertexBuffer = device->createDynamicBuffer(vertexLength);
        if (!indexBuffer)
            indexBuffer = device->createDynamicBuffer(indexLength);

        if (!texture && mPendingAtlasSize > 0)
        {
            TextureOptions options;
            options.width = mPendingAtlasSize;
            options.height = mPendingAtlasSize;
            options.mipCount = 1;
            options.format = Format::R8G8B8A8_UNORM;
            texture = device->createTexture(options);
            if (material)
                material->setParameter(kUIMainTexture, texture);
        }

        Primitive::createResources(device);
    }

    void UIShape::encode(CommandBuffer::Ref cmd)
    {
        if (mTextureDirty && texture && cmd)
        {
            cmd->fillTexture(texture, mPendingUploadRgba);
            mTextureDirty = false;
        }
        if (vertexBuffer && !mVertices.empty())
        {
            void* ptr = vertexBuffer->map();
            memcpy(ptr, mVertices.data(), vertexLength);
            vertexBuffer->unmap();
        }
        if (indexBuffer && !mIndices.empty())
        {
            void* ptr = indexBuffer->map();
            memcpy(ptr, mIndices.data(), indexLength);
            indexBuffer->unmap();
        }
        if (indexCount == 0)
            return;

        cmd->setTopology(Topology::TriangleList);
        cmd->setVertexBuffer(vertexBuffer, vertexLength, vertexStride);
        cmd->setIndexBuffer(indexBuffer, indexLength, indexFormat);
        cmd->drawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }
}
