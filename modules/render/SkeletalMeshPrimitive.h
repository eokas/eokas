#ifndef _EOKAS_RENDER_SKELETAL_MESH_PRIMITIVE_H_
#define _EOKAS_RENDER_SKELETAL_MESH_PRIMITIVE_H_

#include "./Primitive.h"

namespace eokas
{
    // 本轮按 rest GeoMesh 绘制，不调用 Skeleton::skin（该函数目前为空实现）。
    struct SkeletalMeshPrimitive : public Primitive
    {
        using Ref = std::shared_ptr<SkeletalMeshPrimitive>;
        GeoMesh mesh;
        DynamicBuffer::Ref vertexBuffer;
        DynamicBuffer::Ref indexBuffer;
        uint32_t vertexBytes = 0;
        uint32_t vertexStride = 0;
        uint32_t indexBytes = 0;
        uint32_t indexCount = 0;
        Format indexFormat = Format::R32_UINT;
        Skeleton skeleton;
        Skeleton::Pose pose;

        void createResources(Device::Ref device) override;
        void encode(CommandBuffer::Ref cmd) override;
        bool ready() const override;
    };
}

#endif
