#include "./SkeletalMeshPrimitive.h"
#include <cstring>

namespace eokas
{
    void SkeletalMeshPrimitive::createResources(Device::Ref device)
    {
        vertexStride = (uint32_t)sizeof(Vertex);
        vertexBytes = vertexStride * (uint32_t)mesh.vertices.size();
        vertexBuffer = device->createMutableBuffer(vertexBytes);
        {
            void* ptr = vertexBuffer->map();
            memcpy(ptr, mesh.vertices.data(), vertexBytes);
            vertexBuffer->unmap();
        }

        indexFormat = Format::R32_UINT;
        indexCount = (uint32_t)mesh.indices.size();
        indexBytes = indexCount * (uint32_t)sizeof(uint32_t);
        indexBuffer = device->createStaticBuffer(indexBytes);

        Primitive::createResources(device);
    }

    void SkeletalMeshPrimitive::encode(CommandBuffer::Ref cmd)
    {
        if (!indexUploaded)
        {
            cmd->fillBuffer(indexBuffer, mesh.indices.data(), indexBytes);
            indexUploaded = true;
        }
        cmd->setTopology(Topology::TriangleList);
        cmd->setVertexBuffer(vertexBuffer, vertexBytes, vertexStride);
        cmd->setIndexBuffer(indexBuffer, indexBytes, indexFormat);
        cmd->drawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }

    bool SkeletalMeshPrimitive::ready() const
    {
        return material && material->isReady() && vertexBuffer && indexBuffer && indexCount > 0 && objectUniforms;
    }
}
