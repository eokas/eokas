#ifndef _EOKAS_RENDER_MATERIAL_H_
#define _EOKAS_RENDER_MATERIAL_H_

#include "./header.h"

namespace eokas
{
    struct Material
    {
        using Ref = std::shared_ptr<Material>;
        Color albedo = Color(1, 1, 1, 1);
        Texture::Ref albedoTexture;
        std::string shaderSource;

        Program::Ref vs;
        Program::Ref ps;
        PipelineObject::Ref pipelineObject;
        PipelineBindings::Ref pipelineBindings;
        DynamicBuffer::Ref materialUniforms;
        Texture::Ref defaultTexture;
        bool built = false;
        bool textureUploaded = false;

        void build(Device::Ref device, DynamicBuffer::Ref spaceLighting);
        void updateUniforms();
        void uploadDefaultTexture(CommandBuffer::Ref cmd);
    };
}

#endif
