#include "./Primitive.h"
#include <cstring>

namespace eokas
{
    void Primitive::createResources(Device::Ref device)
    {
        if (!objectUniforms)
            objectUniforms = device->createDynamicBuffer(sizeof(TransformUniforms));
    }

    void Primitive::updateUniforms(const Camera& cam)
    {
        if (!objectUniforms)
            return;
        TransformUniforms u;
        u.world = transform.toMatrix();
        if (screenSpace)
        {
            float width = cam.viewport.right - cam.viewport.left;
            float height = cam.viewport.bottom - cam.viewport.top;
            if (width <= 0.0f)
                width = 1.0f;
            if (height <= 0.0f)
                height = 1.0f;
            Matrix4 proj = Matrix4::IDENTITY;
            proj.value[0][0] = 2.0f / width;
            proj.value[1][1] = -2.0f / height;
            proj.value[3][0] = -1.0f;
            proj.value[3][1] = 1.0f;
            u.wvp = Matrix4::transform(u.world, proj);
        }
        else
        {
            u.wvp = Matrix4::transform(u.world, cam.viewProjectionMatrix());
        }
        u.cameraPos[0] = cam.transform.position.x;
        u.cameraPos[1] = cam.transform.position.y;
        u.cameraPos[2] = cam.transform.position.z;
        u.cameraPos[3] = 1.0f;
        void* ptr = objectUniforms->map();
        memcpy(ptr, &u, sizeof(u));
        objectUniforms->unmap();
    }

    bool Primitive::ready() const
    {
        return material && material->isReady() && objectUniforms;
    }
}
