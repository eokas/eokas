#ifndef _EOKAS_RENDER_STATIC_MESH_PRIMITIVE_H_
#define _EOKAS_RENDER_STATIC_MESH_PRIMITIVE_H_

#include "./Primitive.h"

namespace eokas
{
    struct StaticMeshPrimitive : public Primitive
    {
        using Ref = std::shared_ptr<StaticMeshPrimitive>;
        GeoMesh mesh;
        StaticBuffer::Ref vertexBuffer;
        StaticBuffer::Ref indexBuffer;
        uint32_t vertexBytes = 0;
        uint32_t vertexStride = 0;
        uint32_t indexBytes = 0;
        uint32_t indexCount = 0;
        Format indexFormat = Format::R32_UINT;
        bool geometryUploaded = false;

        void createResources(Device::Ref device) override;
        void encode(CommandBuffer::Ref cmd) override;
        bool ready() const override;
    };
}

#endif
