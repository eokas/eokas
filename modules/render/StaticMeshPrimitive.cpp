#include "./StaticMeshPrimitive.h"
#include <cstring>

namespace eokas
{
    void StaticMeshPrimitive::createResources(Device::Ref device)
    {
        vertexStride = (uint32_t)sizeof(Vertex);
        vertexBytes = vertexStride * (uint32_t)mesh.vertices.size();
        vertexBuffer = device->createStaticBuffer(vertexBytes);

        indexFormat = Format::R32_UINT;
        indexCount = (uint32_t)mesh.indices.size();
        indexBytes = indexCount * (uint32_t)sizeof(uint32_t);
        indexBuffer = device->createStaticBuffer(indexBytes);

        Primitive::createResources(device);
    }

    void StaticMeshPrimitive::encode(CommandBuffer::Ref cmd)
    {
        if (!geometryUploaded)
        {
            cmd->fillBuffer(vertexBuffer, mesh.vertices.data(), vertexBytes);
            cmd->fillBuffer(indexBuffer, mesh.indices.data(), indexBytes);
            geometryUploaded = true;
        }
        cmd->setTopology(Topology::TriangleList);
        cmd->setVertexBuffer(vertexBuffer, vertexBytes, vertexStride);
        cmd->setIndexBuffer(indexBuffer, indexBytes, indexFormat);
        cmd->drawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }

    bool StaticMeshPrimitive::ready() const
    {
        return material && material->built && vertexBuffer && indexBuffer && indexCount > 0 && objectUniforms;
    }
}
