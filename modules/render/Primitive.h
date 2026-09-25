#ifndef _EOKAS_RENDER_PRIMITIVE_H_
#define _EOKAS_RENDER_PRIMITIVE_H_

#include "./Camera.h"
#include "./Material.h"

namespace eokas
{
    struct Primitive
    {
        using Ref = std::shared_ptr<Primitive>;
        Transform transform;
        Material::Ref material;
        bool visible = true;
        bool screenSpace = false;
        DynamicBuffer::Ref objectUniforms;

        virtual ~Primitive() = default;
        virtual void createResources(Device::Ref device);
        virtual void updateUniforms(const Camera& cam);
        virtual void upload(CommandBuffer::Ref cmd) { (void)cmd; }
        virtual void encode(CommandBuffer::Ref cmd) = 0;
        virtual bool ready() const;
    };
}

#endif
