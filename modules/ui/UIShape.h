#ifndef _EOKAS_UI_SHAPE_H_
#define _EOKAS_UI_SHAPE_H_

#include "header.h"
#include <vector>

namespace eokas
{
    class UIShape
    {
    public:
        DynamicBuffer::Ref vertexBuffer;
        uint32_t vertexLength = 0;
        uint32_t vertexStride = sizeof(UIVertex);
        DynamicBuffer::Ref indexBuffer;
        uint32_t indexLength = 0;
        Format indexFormat = Format::R32_UINT;
        DynamicBuffer::Ref uniformBuffer;
        Texture::Ref texture;
        uint32_t indexCount = 0;

        void create(Device::Ref device, uint32_t maxQuads = 2048);
        void begin();
        void setTexture(Texture::Ref tex);
        void setProjection(const Matrix4& proj);
        void addQuad(const Rect& screen, const Rect& uv, const Color& color);
        void end();

    private:
        std::vector<UIVertex> mVertices;
        std::vector<uint32_t> mIndices;
        Matrix4 mProjection = Matrix4::IDENTITY;
        uint32_t mMaxQuads = 0;
    };
}

#endif//_EOKAS_UI_SHAPE_H_
