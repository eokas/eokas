#include "UIShape.h"
#include <cstring>
#include <stdexcept>

namespace eokas
{
    void UIShape::create(Device::Ref device, uint32_t maxQuads)
    {
        mMaxQuads = maxQuads;
        vertexStride = sizeof(UIVertex);
        vertexLength = maxQuads * 4 * vertexStride;
        indexFormat = Format::R32_UINT;
        indexLength = maxQuads * 6 * sizeof(uint32_t);
        vertexBuffer = device->createDynamicBuffer(vertexLength, 0);
        indexBuffer = device->createDynamicBuffer(indexLength, 0);
        uniformBuffer = device->createDynamicBuffer(sizeof(Matrix4), (uint32_t)BufferUsage::UniformBuffer);
        mProjection = Matrix4::IDENTITY;
        indexCount = 0;
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

    void UIShape::setProjection(const Matrix4& proj)
    {
        mProjection = proj;
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
        if (uniformBuffer)
        {
            void* ptr = uniformBuffer->map();
            memcpy(ptr, mProjection.value, sizeof(mProjection.value));
            uniformBuffer->unmap();
        }
    }
}
