#ifndef _EOKAS_RENDER_SPACE_H_
#define _EOKAS_RENDER_SPACE_H_

#include "./Light.h"
#include "./Primitive.h"

namespace eokas
{
    struct Space
    {
        using Ref = std::shared_ptr<Space>;
        std::vector<Camera::Ref> cameras;
        Camera::Ref view;
        std::vector<Light::Ref> lights;
        std::vector<Primitive::Ref> primitives;
        Color clearColor = Color(0, 0.2f, 0.4f, 1);
        Color ambient = Color(0.15f, 0.15f, 0.15f, 1);
        Viewport viewport;
        DynamicBuffer::Ref lightingUniforms;
        CommandBuffer::Ref commandBuffer;

        void add(Camera::Ref camera);
        void add(Light::Ref light);
        void add(Primitive::Ref primitive);
        void remove(Camera::Ref camera);
        void remove(Light::Ref light);
        void remove(Primitive::Ref primitive);

        void render(Device::Ref device);

    private:
        void ensureResources(Device::Ref device);
        void updateLighting();
    };
}

#endif
