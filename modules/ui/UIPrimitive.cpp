#include "UIPrimitive.h"
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
            float limit = clip.origin.x;
            if (edge == 1)
            {
                limit = clip.origin.x + clip.size.x;
            }
            else if (edge == 2)
            {
                limit = clip.origin.y;
            }
            else if (edge == 3)
            {
                limit = clip.origin.y + clip.size.y;
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
            if (clip.size.x <= 0.0f || clip.size.y <= 0.0f)
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

    UIPrimitive::UIPrimitive()
    {
        screenSpace = true;
    }

    void UIPrimitive::begin()
    {
        mVertices.clear();
        mIndices.clear();
        indexCount = 0;
        mClipStack.clear();
    }

    void UIPrimitive::pushClip(const Rect& screenClip)
    {
        Rect next = screenClip;
        if (!mClipStack.empty())
        {
            const Rect& prev = mClipStack.back();
            float x0 = greater(prev.origin.x, screenClip.origin.x);
            float y0 = greater(prev.origin.y, screenClip.origin.y);
            float x1 = lesser(prev.origin.x + prev.size.x, screenClip.origin.x + screenClip.size.x);
            float y1 = lesser(prev.origin.y + prev.size.y, screenClip.origin.y + screenClip.size.y);
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

    void UIPrimitive::popClip()
    {
        if (!mClipStack.empty())
        {
            mClipStack.pop_back();
        }
    }

    bool UIPrimitive::outsideClip(const Rect& screen) const
    {
        if (mClipStack.empty())
        {
            return false;
        }
        const Rect& clip = mClipStack.back();
        if (clip.size.x <= 0.0f || clip.size.y <= 0.0f)
        {
            return true;
        }
        float x0 = screen.origin.x;
        float y0 = screen.origin.y;
        float x1 = x0 + screen.size.x;
        float y1 = y0 + screen.size.y;
        float cx1 = clip.origin.x + clip.size.x;
        float cy1 = clip.origin.y + clip.size.y;
        return x1 < clip.origin.x || x0 > cx1 || y1 < clip.origin.y || y0 > cy1;
    }

    void UIPrimitive::setTexture(Texture::Ref tex)
    {
        texture = tex;
    }

    void UIPrimitive::setPendingUpload(const std::vector<uint8_t>& rgba, uint32_t atlasSize)
    {
        mPendingUploadRgba = rgba;
        mPendingAtlasSize = atlasSize;
        mTextureDirty = true;
    }

    void UIPrimitive::addQuad(const Matrix3& world, const Rect& local, const Rect& uv, const Color& color)
    {
        Vector2 origin = local.origin;
        Vector2 size = local.size;
        this->addQuad(
            UIShape::transformPoint(world, origin),
            UIShape::transformPoint(world, Vector2(origin.x + size.x, origin.y)),
            UIShape::transformPoint(world, origin + size),
            UIShape::transformPoint(world, Vector2(origin.x, origin.y + size.y)),
            uv,
            color);
    }

    void UIPrimitive::addQuad(const Rect& screen, const Rect& uv, const Color& color)
    {
        this->addQuad(
            Vector2(screen.origin.x, screen.origin.y),
            Vector2(screen.origin.x + screen.size.x, screen.origin.y),
            Vector2(screen.origin.x + screen.size.x, screen.origin.y + screen.size.y),
            Vector2(screen.origin.x, screen.origin.y + screen.size.y),
            uv,
            color);
    }

    void UIPrimitive::addQuad(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, const Rect& uv, const Color& color)
    {
        Vector4 vertexColor(color.r, color.g, color.b, color.a);
        ClipVert q0 { p0.x, p0.y, uv.origin.x, uv.origin.y };
        ClipVert q1 { p1.x, p1.y, uv.origin.x + uv.size.x, uv.origin.y };
        ClipVert q2 { p2.x, p2.y, uv.origin.x + uv.size.x, uv.origin.y + uv.size.y };
        ClipVert q3 { p3.x, p3.y, uv.origin.x, uv.origin.y + uv.size.y };

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

    void UIPrimitive::addTriangles(const Vector2* vertices, uint32_t triangleCount, const Rect& uv, const Color& color)
    {
        if (vertices == nullptr || triangleCount == 0)
        {
            return;
        }
        Vector4 vertexColor(color.r, color.g, color.b, color.a);
        float u0 = uv.origin.x;
        float v0 = uv.origin.y;
        float u1 = uv.origin.x + uv.size.x;
        float v1 = uv.origin.y + uv.size.y;
        for (uint32_t i = 0; i < triangleCount; ++i)
        {
            const Vector2& p0 = vertices[i * 3];
            const Vector2& p1 = vertices[i * 3 + 1];
            const Vector2& p2 = vertices[i * 3 + 2];
            ClipVert c0 { p0.x, p0.y, u0, v0 };
            ClipVert c1 { p1.x, p1.y, u1, v0 };
            ClipVert c2 { p2.x, p2.y, u1, v1 };
            if (mClipStack.empty())
            {
                emitTriangle(mVertices, mIndices, c0, c1, c2, vertexColor);
            }
            else
            {
                emitClippedTriangle(mVertices, mIndices, c0, c1, c2, mClipStack.back(), vertexColor);
            }
        }
    }

    void UIPrimitive::reserveGeometry()
    {
        uint32_t vertQuads = (uint32_t)((mVertices.size() + 3) / 4);
        uint32_t indexQuads = (uint32_t)((mIndices.size() + 5) / 6);
        uint32_t needed = vertQuads > indexQuads ? vertQuads : indexQuads;
        if (needed <= mMaxQuads)
        {
            return;
        }
        uint32_t cap = mMaxQuads < 1 ? 1u : mMaxQuads;
        while (cap < needed)
        {
            if (cap > 0x7fffffffu / 2u)
            {
                cap = needed;
                break;
            }
            cap *= 2u;
        }
        mMaxQuads = cap;
    }

    void UIPrimitive::ensureGpuBuffers()
    {
        if (!mDevice)
        {
            return;
        }
        vertexStride = sizeof(UIVertex);
        indexFormat = Format::R32_UINT;
        uint64_t vertexBytes64 = (uint64_t)mMaxQuads * 4ull * (uint64_t)vertexStride;
        uint64_t indexBytes64 = (uint64_t)mMaxQuads * 6ull * (uint64_t)sizeof(uint32_t);
        if (vertexBytes64 > 0xffffffffull || indexBytes64 > 0xffffffffull)
        {
            throw std::runtime_error("UIPrimitive: geometry exceeds GPU buffer limits.");
        }
        uint32_t vertexBytes = (uint32_t)vertexBytes64;
        uint32_t indexBytes = (uint32_t)indexBytes64;
        if (!vertexBuffer || vertexBuffer->getLength() < vertexBytes)
        {
            if (vertexBuffer)
            {
                mRetired.push_back({ vertexBuffer, (int)kFrameCount });
            }
            vertexBuffer = mDevice->createDynamicBuffer(vertexBytes);
        }
        if (!indexBuffer || indexBuffer->getLength() < indexBytes)
        {
            if (indexBuffer)
            {
                mRetired.push_back({ indexBuffer, (int)kFrameCount });
            }
            indexBuffer = mDevice->createDynamicBuffer(indexBytes);
        }
    }

    void UIPrimitive::flushRetired()
    {
        for (size_t i = 0; i < mRetired.size();)
        {
            mRetired[i].frames -= 1;
            if (mRetired[i].frames <= 0)
            {
                mRetired.erase(mRetired.begin() + (ptrdiff_t)i);
                continue;
            }
            i += 1;
        }
    }

    void UIPrimitive::end()
    {
        this->reserveGeometry();
        indexCount = (uint32_t)mIndices.size();
        vertexLength = (uint32_t)(mVertices.size() * sizeof(UIVertex));
        indexLength = (uint32_t)(mIndices.size() * sizeof(uint32_t));
    }

    void UIPrimitive::createResources(Device::Ref device)
    {
        mDevice = device;
        this->ensureGpuBuffers();

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

    void UIPrimitive::upload(CommandBuffer::Ref cmd)
    {
        if (!mTextureDirty || !texture || !cmd)
        {
            return;
        }
        cmd->fillTexture(texture, mPendingUploadRgba);
        mTextureDirty = false;
    }

    void UIPrimitive::encode(CommandBuffer::Ref cmd)
    {
        this->flushRetired();
        this->ensureGpuBuffers();
        uint32_t vertexBytes = (uint32_t)(mVertices.size() * sizeof(UIVertex));
        uint32_t indexBytes = (uint32_t)(mIndices.size() * sizeof(uint32_t));
        if (vertexBuffer && vertexBytes > 0)
        {
            void* ptr = vertexBuffer->map();
            memcpy(ptr, mVertices.data(), vertexBytes);
            vertexBuffer->unmap();
        }
        if (indexBuffer && indexBytes > 0)
        {
            void* ptr = indexBuffer->map();
            memcpy(ptr, mIndices.data(), indexBytes);
            indexBuffer->unmap();
        }
        if (indexCount == 0)
            return;

        cmd->setTopology(Topology::TriangleList);
        cmd->setVertexBuffer(vertexBuffer, vertexBytes, vertexStride);
        cmd->setIndexBuffer(indexBuffer, indexBytes, indexFormat);
        cmd->drawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }
}
