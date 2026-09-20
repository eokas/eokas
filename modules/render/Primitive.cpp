#include "./Primitive.h"
#include <cstring>

namespace eokas
{
    void Primitive::createResources(Device::Ref device)
    {
        if (!objectUniforms)
            objectUniforms = device->createDynamicBuffer(sizeof(TransformUniforms), (uint32_t)BufferUsage::UniformBuffer);
    }

    void Primitive::updateUniforms(const Camera& cam)
    {
        if (!objectUniforms)
            return;
        TransformUniforms u;
        u.world = transform.toMatrix();
        u.wvp = Matrix4::transform(u.world, cam.viewProjectionMatrix());
        u.cameraPos[0] = cam.transform.position.x;
        u.cameraPos[1] = cam.transform.position.y;
        u.cameraPos[2] = cam.transform.position.z;
        u.cameraPos[3] = 1.0f;
        void* ptr = objectUniforms->map();
        memcpy(ptr, &u, sizeof(u));
        objectUniforms->unmap();
    }
}
