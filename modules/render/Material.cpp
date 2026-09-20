#include "./Material.h"
#include <cstddef>
#include <cstring>
#include <stdexcept>

namespace eokas
{
    static uint32_t paramBytes(MaterialParamType type)
    {
        switch (type)
        {
            case MaterialParamType::Float: return 4;
            case MaterialParamType::Float2: return 8;
            case MaterialParamType::Float3: return 12;
            case MaterialParamType::Float4: return 16;
        }
        return 0;
    }

    static bool writeParam(MaterialUniforms& data, const MaterialParameter* param, MaterialParamType type, const float* values)
    {
        if (!param || param->type != type)
            return false;
        memcpy(reinterpret_cast<uint8_t*>(&data) + param->offset, values, paramBytes(type));
        return true;
    }

    static bool readParam(const MaterialUniforms& data, const MaterialParameter* param, MaterialParamType type, float* values)
    {
        if (!param || param->type != type)
            return false;
        memcpy(values, reinterpret_cast<const uint8_t*>(&data) + param->offset, paramBytes(type));
        return true;
    }

    static String resolveShaderPath(const std::string& relativePath)
    {
        String given = relativePath.c_str();
        if (File::exists(given))
            return given;

        String exeDir = File::basePath(Process::executingPath());
        String fromExe = File::combinePath(exeDir, given);
        if (File::exists(fromExe))
            return fromExe;

        String fromBuildDir = File::combinePath(File::basePath(exeDir), given);
        if (File::exists(fromBuildDir))
            return fromBuildDir;

        return given;
    }

    static std::string loadShaderSource(const std::string& shaderSource, const std::string& shaderPath)
    {
        if (!shaderSource.empty())
            return shaderSource;
        if (shaderPath.empty())
            throw std::runtime_error("Material: shaderSource and shaderPath are empty.");

        String path = resolveShaderPath(shaderPath);
        FileStream stream(path, "rb");
        if (!stream.open())
            throw std::runtime_error("Material: failed to read shader.");
        size_t size = stream.size();
        std::string src(size, '\0');
        if (size > 0)
            stream.read(src.data(), size);
        stream.close();
        if (src.size() >= 3 && (uint8_t)src[0] == 0xEF && (uint8_t)src[1] == 0xBB && (uint8_t)src[2] == 0xBF)
            src.erase(0, 3);
        return src;
    }

    MaterialParameterBlock::MaterialParameterBlock()
        : name("MaterialParams")
        , data{}
        , buffer()
        , parameters()
    {
        data.albedo[0] = 1.0f;
        data.albedo[1] = 1.0f;
        data.albedo[2] = 1.0f;
        data.albedo[3] = 1.0f;
        data.metallic = 0.0f;
        data.roughness = 0.5f;
        data.ao = 1.0f;
        data._pad0 = 0.0f;

        parameters.push_back({"albedo", MaterialParamType::Float4, (uint32_t)offsetof(MaterialUniforms, albedo)});
        parameters.push_back({"metallic", MaterialParamType::Float, (uint32_t)offsetof(MaterialUniforms, metallic)});
        parameters.push_back({"roughness", MaterialParamType::Float, (uint32_t)offsetof(MaterialUniforms, roughness)});
        parameters.push_back({"ao", MaterialParamType::Float, (uint32_t)offsetof(MaterialUniforms, ao)});
        parameters.push_back({"emissive", MaterialParamType::Float4, (uint32_t)offsetof(MaterialUniforms, emissive)});
    }

    MaterialParameter* MaterialParameterBlock::findParameter(const std::string& name)
    {
        for (auto& p : parameters)
        {
            if (p.name == name)
                return &p;
        }
        return nullptr;
    }

    const MaterialParameter* MaterialParameterBlock::findParameter(const std::string& name) const
    {
        for (const auto& p : parameters)
        {
            if (p.name == name)
                return &p;
        }
        return nullptr;
    }

    bool MaterialParameterBlock::setParameter(const std::string& name, float value)
    {
        float v[1] = {value};
        return writeParam(data, findParameter(name), MaterialParamType::Float, v);
    }

    bool MaterialParameterBlock::setParameter(const std::string& name, const Vector2& value)
    {
        float v[2] = {value.x, value.y};
        return writeParam(data, findParameter(name), MaterialParamType::Float2, v);
    }

    bool MaterialParameterBlock::setParameter(const std::string& name, const Vector3& value)
    {
        float v[3] = {value.x, value.y, value.z};
        return writeParam(data, findParameter(name), MaterialParamType::Float3, v);
    }

    bool MaterialParameterBlock::setParameter(const std::string& name, const Vector4& value)
    {
        float v[4] = {value.x, value.y, value.z, value.w};
        return writeParam(data, findParameter(name), MaterialParamType::Float4, v);
    }

    bool MaterialParameterBlock::setParameter(const std::string& name, const Color& value)
    {
        float v[4] = {value.r, value.g, value.b, value.a};
        return writeParam(data, findParameter(name), MaterialParamType::Float4, v);
    }

    bool MaterialParameterBlock::getParameter(const std::string& name, float& out) const
    {
        float v[1] = {0};
        if (!readParam(data, findParameter(name), MaterialParamType::Float, v))
            return false;
        out = v[0];
        return true;
    }

    bool MaterialParameterBlock::getParameter(const std::string& name, Vector2& out) const
    {
        float v[2] = {0, 0};
        if (!readParam(data, findParameter(name), MaterialParamType::Float2, v))
            return false;
        out = Vector2(v[0], v[1]);
        return true;
    }

    bool MaterialParameterBlock::getParameter(const std::string& name, Vector3& out) const
    {
        float v[3] = {0, 0, 0};
        if (!readParam(data, findParameter(name), MaterialParamType::Float3, v))
            return false;
        out = Vector3(v[0], v[1], v[2]);
        return true;
    }

    bool MaterialParameterBlock::getParameter(const std::string& name, Vector4& out) const
    {
        float v[4] = {0, 0, 0, 0};
        if (!readParam(data, findParameter(name), MaterialParamType::Float4, v))
            return false;
        out = Vector4(v[0], v[1], v[2], v[3]);
        return true;
    }

    bool MaterialParameterBlock::getParameter(const std::string& name, Color& out) const
    {
        float v[4] = {0, 0, 0, 0};
        if (!readParam(data, findParameter(name), MaterialParamType::Float4, v))
            return false;
        out = Color(v[0], v[1], v[2], v[3]);
        return true;
    }

    void MaterialParameterBlock::createBuffer(Device::Ref device)
    {
        if (buffer || !device)
            return;
        buffer = device->createDynamicBuffer(sizeof(MaterialUniforms));
        update();
    }

    void MaterialParameterBlock::update()
    {
        if (!buffer)
            return;
        void* ptr = buffer->map();
        memcpy(ptr, &data, sizeof(data));
        buffer->unmap();
    }

    bool Material::setParameter(const std::string& name, float value)
    {
        return parameterBlock.setParameter(name, value);
    }

    bool Material::setParameter(const std::string& name, const Vector2& value)
    {
        return parameterBlock.setParameter(name, value);
    }

    bool Material::setParameter(const std::string& name, const Vector3& value)
    {
        return parameterBlock.setParameter(name, value);
    }

    bool Material::setParameter(const std::string& name, const Vector4& value)
    {
        return parameterBlock.setParameter(name, value);
    }

    bool Material::setParameter(const std::string& name, const Color& value)
    {
        return parameterBlock.setParameter(name, value);
    }

    bool Material::setParameter(const std::string& name, Texture::Ref texture)
    {
        if (built && pipelineBindings)
        {
            if (!pipelineBindings->getLayout().findByName(PipelineResourceType::Texture, name))
                return false;
            pipelineBindings->setTextureByName(name, texture ? texture : defaultTexture);
        }
        for (auto& item : textures)
        {
            if (item.first == name)
            {
                item.second = texture;
                return true;
            }
        }
        textures.push_back({name, texture});
        return true;
    }

    bool Material::getParameter(const std::string& name, float& out) const
    {
        return parameterBlock.getParameter(name, out);
    }

    bool Material::getParameter(const std::string& name, Vector2& out) const
    {
        return parameterBlock.getParameter(name, out);
    }

    bool Material::getParameter(const std::string& name, Vector3& out) const
    {
        return parameterBlock.getParameter(name, out);
    }

    bool Material::getParameter(const std::string& name, Vector4& out) const
    {
        return parameterBlock.getParameter(name, out);
    }

    bool Material::getParameter(const std::string& name, Color& out) const
    {
        return parameterBlock.getParameter(name, out);
    }

    bool Material::getParameter(const std::string& name, Texture::Ref& out) const
    {
        for (const auto& item : textures)
        {
            if (item.first == name)
            {
                out = item.second;
                return true;
            }
        }
        return false;
    }

    void Material::build(Device::Ref device, DynamicBuffer::Ref spaceLighting)
    {
        if (built)
            return;

        std::string src = loadShaderSource(shaderSource, shaderPath);
        std::string programName = "BPR";
        if (!shaderPath.empty())
            programName = File::fileNameWithoutExtension(resolveShaderPath(shaderPath)).cstr();
        else if (!shaderSource.empty())
            programName = "material";

        ProgramOptions vsOpt;
        vsOpt.name = programName + "-vs";
        vsOpt.source = src;
        vsOpt.entry = "VSMain";
        vsOpt.type = ProgramType::Vertex;
        vsOpt.target = ProgramTarget::SM_5_0;
        vs = device->createProgram(vsOpt);

        ProgramOptions psOpt;
        psOpt.name = programName + "-ps";
        psOpt.source = src;
        psOpt.entry = "PSMain";
        psOpt.type = ProgramType::Fragment;
        psOpt.target = ProgramTarget::SM_5_0;
        ps = device->createProgram(psOpt);

        std::vector<VertexElement> vElements;
        vElements.push_back({"POSITION", 0, 0, Format::R32G32B32_FLOAT});
        vElements.push_back({"NORMAL", 0, 12, Format::R32G32B32_FLOAT});
        vElements.push_back({"COLOR", 0, 24, Format::R32G32B32A32_FLOAT});
        vElements.push_back({"TEXCOORD", 0, 40, Format::R32G32_FLOAT});

        pipelineObject = device->createPipelineObject();
        pipelineObject->begin();
        pipelineObject->setProgram(ProgramType::Vertex, vs);
        pipelineObject->setProgram(ProgramType::Fragment, ps);
        pipelineObject->setVertexElements(vElements);
        pipelineObject->setCullMode(CullMode::Front);
        DepthStencilState depthStencil;
        depthStencil.depthTest = true;
        depthStencil.depthWrite = true;
        depthStencil.depthFunc = CompareOp::Less;
        pipelineObject->setDepthStencilState(depthStencil);
        SamplerState sampler;
        sampler.minFilter = SamplerFilterMode::Linear;
        sampler.magFilter = SamplerFilterMode::Linear;
        sampler.mipFilter = SamplerFilterMode::Linear;
        sampler.addressU = SamplerAddressMode::Clamp;
        sampler.addressV = SamplerAddressMode::Clamp;
        sampler.addressW = SamplerAddressMode::Clamp;
        pipelineObject->setSamplerState(0, sampler);
        BlendState blend;
        blend.enabled = true;
        blend.srcColor = BlendFactor::SrcAlpha;
        blend.dstColor = BlendFactor::OneMinusSrcAlpha;
        blend.colorOp = BlendOp::Add;
        blend.srcAlpha = BlendFactor::One;
        blend.dstAlpha = BlendFactor::OneMinusSrcAlpha;
        blend.alphaOp = BlendOp::Add;
        pipelineObject->setBlendState(blend);
        pipelineObject->end();

        parameterBlock.createBuffer(device);

        TextureOptions options;
        options.width = 1;
        options.height = 1;
        options.mipCount = 1;
        options.format = Format::R8G8B8A8_UNORM;
        defaultTexture = device->createTexture(options);

        pipelineBindings = device->createPipelineBindings(pipelineObject);
        pipelineBindings->begin();
        pipelineBindings->setUniformBufferByName("Lighting", spaceLighting);
        pipelineBindings->setUniformBufferByName(parameterBlock.name, parameterBlock.buffer);
        const PipelineLayout& layout = pipelineBindings->getLayout();
        for (const auto& entry : layout.entries)
        {
            if (entry.type != PipelineResourceType::Texture)
                continue;
            Texture::Ref tex = defaultTexture;
            for (const auto& item : textures)
            {
                if (item.first == entry.name && item.second)
                {
                    tex = item.second;
                    break;
                }
            }
            pipelineBindings->setTextureByName(entry.name, tex);
        }
        pipelineBindings->end();

        built = true;
    }

    void Material::updateUniforms()
    {
        parameterBlock.update();
    }

    void Material::uploadDefaultTexture(CommandBuffer::Ref cmd)
    {
        if (textureUploaded || !cmd)
            return;
        if (defaultTexture)
        {
            std::vector<uint8_t> white = {255, 255, 255, 255};
            cmd->fillTexture(defaultTexture, white);
        }
        textureUploaded = true;
    }
}
