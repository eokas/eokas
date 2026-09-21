#include "UIShape.h"
#include <cstring>
#include <stdexcept>

namespace eokas
{
    UIShape::UIShape()
    {
        screenSpace = true;
    }

    void UIShape::begin()
    {
        mVertices.clear();
        mIndices.clear();
        indexCount = 0;
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
        uint32_t base = (uint32_t)mVertices.size();
        Vector4 vertexColor(color.r, color.g, color.b, color.a);

        mVertices.push_back({ Vector2(screen.x, screen.y), Vector2(uv.x, uv.y), vertexColor });
        mVertices.push_back({ Vector2(screen.x + screen.width, screen.y), Vector2(uv.x + uv.width, uv.y), vertexColor });
        mVertices.push_back({ Vector2(screen.x + screen.width, screen.y + screen.height), Vector2(uv.x + uv.width, uv.y + uv.height), vertexColor });
        mVertices.push_back({ Vector2(screen.x, screen.y + screen.height), Vector2(uv.x, uv.y + uv.height), vertexColor });

        mIndices.push_back(base + 0);
        mIndices.push_back(base + 1);
        mIndices.push_back(base + 2);
        mIndices.push_back(base + 0);
        mIndices.push_back(base + 2);
        mIndices.push_back(base + 3);
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
                material->setParameter("gMainTexture", texture);
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
