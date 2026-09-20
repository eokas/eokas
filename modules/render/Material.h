#ifndef _EOKAS_RENDER_MATERIAL_H_
#define _EOKAS_RENDER_MATERIAL_H_

#include "./header.h"

namespace eokas
{
    enum class MaterialParamType
    {
        Float, Float2, Float3, Float4
    };

    struct MaterialParameter
    {
        std::string name;
        MaterialParamType type = MaterialParamType::Float4;
        uint32_t offset = 0;
    };

    struct MaterialParameterBlock
    {
        std::string name = "MaterialParams";
        MaterialUniforms data;
        DynamicBuffer::Ref buffer;
        std::vector<MaterialParameter> parameters;

        MaterialParameterBlock();

        MaterialParameter* findParameter(const std::string& name);
        const MaterialParameter* findParameter(const std::string& name) const;

        bool setParameter(const std::string& name, float value);
        bool setParameter(const std::string& name, const Vector2& value);
        bool setParameter(const std::string& name, const Vector3& value);
        bool setParameter(const std::string& name, const Vector4& value);
        bool setParameter(const std::string& name, const Color& value);

        bool getParameter(const std::string& name, float& out) const;
        bool getParameter(const std::string& name, Vector2& out) const;
        bool getParameter(const std::string& name, Vector3& out) const;
        bool getParameter(const std::string& name, Vector4& out) const;
        bool getParameter(const std::string& name, Color& out) const;

        void createBuffer(Device::Ref device);
        void update();
    };

    struct Material
    {
        using Ref = std::shared_ptr<Material>;
        std::string shaderPath;
        std::string shaderSource;
        MaterialParameterBlock parameterBlock;
        std::vector<std::pair<std::string, Texture::Ref>> textures;

        Program::Ref vs;
        Program::Ref ps;
        PipelineObject::Ref pipelineObject;
        PipelineBindings::Ref pipelineBindings;
        Texture::Ref defaultTexture;
        bool built = false;
        bool textureUploaded = false;

        bool setParameter(const std::string& name, float value);
        bool setParameter(const std::string& name, const Vector2& value);
        bool setParameter(const std::string& name, const Vector3& value);
        bool setParameter(const std::string& name, const Vector4& value);
        bool setParameter(const std::string& name, const Color& value);
        bool setParameter(const std::string& name, Texture::Ref texture);

        bool getParameter(const std::string& name, float& out) const;
        bool getParameter(const std::string& name, Vector2& out) const;
        bool getParameter(const std::string& name, Vector3& out) const;
        bool getParameter(const std::string& name, Vector4& out) const;
        bool getParameter(const std::string& name, Color& out) const;
        bool getParameter(const std::string& name, Texture::Ref& out) const;

        void build(Device::Ref device, DynamicBuffer::Ref spaceLighting);
        void updateUniforms();
        void uploadDefaultTexture(CommandBuffer::Ref cmd);
    };
}

#endif
