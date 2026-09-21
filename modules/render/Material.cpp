#include "./Material.h"
#include <cstddef>
#include <cstring>

namespace eokas
{
    static const char* kMaterialParamsName = "MaterialParams";

    enum class MaterialParamType
    {
        Float, Float2, Float3, Float4
    };

    struct MaterialParameter
    {
        const char* name;
        MaterialParamType type;
        uint32_t offset;
    };

    static const MaterialParameter kUniformParams[] =
    {
        {"albedo", MaterialParamType::Float4, (uint32_t)offsetof(MaterialUniforms, albedo)},
        {"metallic", MaterialParamType::Float, (uint32_t)offsetof(MaterialUniforms, metallic)},
        {"roughness", MaterialParamType::Float, (uint32_t)offsetof(MaterialUniforms, roughness)},
        {"ao", MaterialParamType::Float, (uint32_t)offsetof(MaterialUniforms, ao)},
        {"emissive", MaterialParamType::Float4, (uint32_t)offsetof(MaterialUniforms, emissive)},
    };

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

    static const MaterialParameter* findUniformParam(const String& name)
    {
        for (const auto& p : kUniformParams)
        {
            if (name == p.name)
                return &p;
        }
        return nullptr;
    }

    static bool writeParam(MaterialUniforms& data, const String& name, MaterialParamType type, const float* values)
    {
        const MaterialParameter* param = findUniformParam(name);
        if (!param || param->type != type)
            return false;
        memcpy(reinterpret_cast<uint8_t*>(&data) + param->offset, values, paramBytes(type));
        return true;
    }

    static bool readParam(const MaterialUniforms& data, const String& name, MaterialParamType type, float* values)
    {
        const MaterialParameter* param = findUniformParam(name);
        if (!param || param->type != type)
            return false;
        memcpy(values, reinterpret_cast<const uint8_t*>(&data) + param->offset, paramBytes(type));
        return true;
    }

    static String resolveShaderPath(const String& relativePath)
    {
        if (File::exists(relativePath))
            return relativePath;

        String exeDir = File::basePath(Process::executingPath());
        String fromExe = File::combinePath(exeDir, relativePath);
        if (File::exists(fromExe))
            return fromExe;

        String fromBuildDir = File::combinePath(File::basePath(exeDir), relativePath);
        if (File::exists(fromBuildDir))
            return fromBuildDir;

        return relativePath;
    }

    static bool loadShaderSource(const String& shaderSource, const String& shaderPath, String& out)
    {
        if (!shaderSource.isEmpty())
        {
            out = shaderSource;
            return true;
        }
        if (shaderPath.isEmpty())
            return false;

        String path = resolveShaderPath(shaderPath);
        FileStream stream(path, "rb");
        if (!stream.open())
            return false;
        size_t size = stream.size();
        out = String(' ', size);
        if (size > 0)
            stream.read((void*)out.cstr(), size);
        stream.close();
        if (out.length() >= 3 && (uint8_t)out.at(0) == 0xEF && (uint8_t)out.at(1) == 0xBB && (uint8_t)out.at(2) == 0xBF)
            out.remove(0, 3);
        return true;
    }

    static void initDefaultUniforms(MaterialUniforms& data)
    {
        data = {};
        data.albedo[0] = 1.0f;
        data.albedo[1] = 1.0f;
        data.albedo[2] = 1.0f;
        data.albedo[3] = 1.0f;
        data.metallic = 0.0f;
        data.roughness = 0.5f;
        data.ao = 1.0f;
    }

    Material::Material()
    {
        initDefaultUniforms(mUniformData);
        mSamplers.push_back({0, SamplerState()});
        mBlend.enabled = true;
        mBlend.srcColor = BlendFactor::SrcAlpha;
        mBlend.dstColor = BlendFactor::OneMinusSrcAlpha;
        mBlend.colorOp = BlendOp::Add;
        mBlend.srcAlpha = BlendFactor::One;
        mBlend.dstAlpha = BlendFactor::OneMinusSrcAlpha;
        mBlend.alphaOp = BlendOp::Add;
    }

    void Material::invalidatePipeline()
    {
        mVS.reset();
        mPS.reset();
        mPipelineObject.reset();
        mPipelineBindings.reset();
        mPipelineDirty = true;
        mBindingsDirty = true;
    }

    void Material::invalidateDeviceResources()
    {
        this->invalidatePipeline();
        mMaterialParamsBuffer.reset();
        mDefaultTexture.reset();
        mTextureUploaded = false;
        mUniformsDirty = true;
    }

    void Material::setDevice(Device::Ref device)
    {
        if (mDevice == device)
            return;
        mDevice = device;
        this->invalidateDeviceResources();
    }

    void Material::setShaderPath(const String& path)
    {
        if (mShaderPath == path)
            return;
        mShaderPath = path;
        this->invalidatePipeline();
    }

    void Material::setShaderSource(const String& source)
    {
        if (mShaderSource == source)
            return;
        mShaderSource = source;
        this->invalidatePipeline();
    }

    void Material::addVertexElement(const VertexElement& element)
    {
        mVertexElements.push_back(element);
        this->invalidatePipeline();
    }

    void Material::setVertexElements(const std::vector<VertexElement>& elements)
    {
        mVertexElements = elements;
        this->invalidatePipeline();
    }

    void Material::setFillMode(FillMode mode)
    {
        if (mFillMode == mode)
            return;
        mFillMode = mode;
        this->invalidatePipeline();
    }

    void Material::setCullMode(CullMode mode)
    {
        if (mCullMode == mode)
            return;
        mCullMode = mode;
        this->invalidatePipeline();
    }

    void Material::setDepthStencilState(const DepthStencilState& state)
    {
        if (mDepthStencil == state)
            return;
        mDepthStencil = state;
        this->invalidatePipeline();
    }

    void Material::setBlendState(const BlendState& state)
    {
        if (mBlend == state)
            return;
        mBlend = state;
        this->invalidatePipeline();
    }

    void Material::setSamplerState(uint32_t index, const SamplerState& state)
    {
        for (auto& item : mSamplers)
        {
            if (item.first == index)
            {
                if (item.second == state)
                    return;
                item.second = state;
                this->invalidatePipeline();
                return;
            }
        }
        mSamplers.push_back({index, state});
        this->invalidatePipeline();
    }

    bool Material::setParameter(const String& name, float value)
    {
        float v[1] = {value};
        if (!writeParam(mUniformData, name, MaterialParamType::Float, v))
            return false;
        mUniformsDirty = true;
        return true;
    }

    bool Material::setParameter(const String& name, const Vector2& value)
    {
        float v[2] = {value.x, value.y};
        if (!writeParam(mUniformData, name, MaterialParamType::Float2, v))
            return false;
        mUniformsDirty = true;
        return true;
    }

    bool Material::setParameter(const String& name, const Vector3& value)
    {
        float v[3] = {value.x, value.y, value.z};
        if (!writeParam(mUniformData, name, MaterialParamType::Float3, v))
            return false;
        mUniformsDirty = true;
        return true;
    }

    bool Material::setParameter(const String& name, const Vector4& value)
    {
        float v[4] = {value.x, value.y, value.z, value.w};
        if (!writeParam(mUniformData, name, MaterialParamType::Float4, v))
            return false;
        mUniformsDirty = true;
        return true;
    }

    bool Material::setParameter(const String& name, const Color& value)
    {
        float v[4] = {value.r, value.g, value.b, value.a};
        if (!writeParam(mUniformData, name, MaterialParamType::Float4, v))
            return false;
        mUniformsDirty = true;
        return true;
    }

    bool Material::isTextureNameValid(const String& name) const
    {
        if (!this->hasLayout())
            return true;
        return mPipelineObject->getLayout().findByName(PipelineResourceType::Texture, name.cstr()) != nullptr;
    }

    bool Material::isUniformBufferNameValid(const String& name) const
    {
        if (!this->hasLayout())
            return true;
        return mPipelineObject->getLayout().findByName(PipelineResourceType::UniformBuffer, name.cstr()) != nullptr;
    }

    bool Material::setParameter(const String& name, Texture::Ref texture)
    {
        if (name.isEmpty() || !this->isTextureNameValid(name))
            return false;

        for (auto& item : mTextures)
        {
            if (item.first == name)
            {
                if (item.second == texture)
                    return true;
                item.second = texture;
                mBindingsDirty = true;
                return true;
            }
        }
        mTextures.push_back({name, texture});
        mBindingsDirty = true;
        return true;
    }

    bool Material::getParameter(const String& name, float& out) const
    {
        float v[1] = {0};
        if (!readParam(mUniformData, name, MaterialParamType::Float, v))
            return false;
        out = v[0];
        return true;
    }

    bool Material::getParameter(const String& name, Vector2& out) const
    {
        float v[2] = {0, 0};
        if (!readParam(mUniformData, name, MaterialParamType::Float2, v))
            return false;
        out = Vector2(v[0], v[1]);
        return true;
    }

    bool Material::getParameter(const String& name, Vector3& out) const
    {
        float v[3] = {0, 0, 0};
        if (!readParam(mUniformData, name, MaterialParamType::Float3, v))
            return false;
        out = Vector3(v[0], v[1], v[2]);
        return true;
    }

    bool Material::getParameter(const String& name, Vector4& out) const
    {
        float v[4] = {0, 0, 0, 0};
        if (!readParam(mUniformData, name, MaterialParamType::Float4, v))
            return false;
        out = Vector4(v[0], v[1], v[2], v[3]);
        return true;
    }

    bool Material::getParameter(const String& name, Color& out) const
    {
        float v[4] = {0, 0, 0, 0};
        if (!readParam(mUniformData, name, MaterialParamType::Float4, v))
            return false;
        out = Color(v[0], v[1], v[2], v[3]);
        return true;
    }

    bool Material::getParameter(const String& name, Texture::Ref& out) const
    {
        if (name.isEmpty() || !this->isTextureNameValid(name))
            return false;
        for (const auto& item : mTextures)
        {
            if (item.first == name)
            {
                out = item.second;
                return true;
            }
        }
        return false;
    }

    bool Material::setUniformBuffer(const String& name, DynamicBuffer::Ref buffer)
    {
        if (name.isEmpty() || name == kMaterialParamsName)
            return false;
        if (!this->isUniformBufferNameValid(name))
            return false;

        for (auto& item : mUniformBuffers)
        {
            if (item.first == name)
            {
                if (item.second == buffer)
                    return true;
                item.second = buffer;
                if (mPipelineBindings && mPipelineBindings->getLayout().findByName(PipelineResourceType::UniformBuffer, name.cstr()))
                    mPipelineBindings->setUniformBufferByName(name.cstr(), buffer);
                return true;
            }
        }

        mUniformBuffers.push_back({name, buffer});
        if (mPipelineBindings && mPipelineBindings->getLayout().findByName(PipelineResourceType::UniformBuffer, name.cstr()))
            mPipelineBindings->setUniformBufferByName(name.cstr(), buffer);
        return true;
    }

    void Material::pruneTextures(const PipelineLayout& layout)
    {
        auto it = mTextures.begin();
        while (it != mTextures.end())
        {
            if (!layout.findByName(PipelineResourceType::Texture, it->first.cstr()))
                it = mTextures.erase(it);
            else
                ++it;
        }
    }

    void Material::flushUniforms()
    {
        if (!mMaterialParamsBuffer)
            return;
        void* ptr = mMaterialParamsBuffer->map();
        memcpy(ptr, &mUniformData, sizeof(mUniformData));
        mMaterialParamsBuffer->unmap();
        mUniformsDirty = false;
    }

    void Material::uploadDefaultTexture(CommandBuffer::Ref cmd)
    {
        if (mTextureUploaded || !cmd || !mDefaultTexture)
            return;
        std::vector<uint8_t> white = {255, 255, 255, 255};
        cmd->fillTexture(mDefaultTexture, white);
        mTextureUploaded = true;
    }

    void Material::bindResources()
    {
        if (!mDevice || !mPipelineObject)
            return;

        mPipelineBindings = mDevice->createPipelineBindings(mPipelineObject);
        mPipelineBindings->begin();
        const PipelineLayout& layout = mPipelineBindings->getLayout();

        if (layout.findByName(PipelineResourceType::UniformBuffer, kMaterialParamsName))
        {
            if (!mMaterialParamsBuffer)
            {
                mMaterialParamsBuffer = mDevice->createDynamicBuffer(sizeof(MaterialUniforms));
                mUniformsDirty = true;
                this->flushUniforms();
            }
            mPipelineBindings->setUniformBufferByName(kMaterialParamsName, mMaterialParamsBuffer);
        }

        for (const auto& item : mUniformBuffers)
        {
            if (item.second && layout.findByName(PipelineResourceType::UniformBuffer, item.first.cstr()))
                mPipelineBindings->setUniformBufferByName(item.first.cstr(), item.second);
        }

        for (const auto& entry : layout.entries)
        {
            if (entry.type != PipelineResourceType::Texture)
                continue;
            Texture::Ref tex = mDefaultTexture;
            for (const auto& item : mTextures)
            {
                if (item.first == entry.name.c_str() && item.second)
                {
                    tex = item.second;
                    break;
                }
            }
            mPipelineBindings->setTextureByName(entry.name, tex);
        }
        mPipelineBindings->end();
        this->pruneTextures(layout);
    }

    bool Material::build()
    {
        if (!mDevice)
            return false;
        if (!mPipelineDirty && !mBindingsDirty && mPipelineObject && mPipelineBindings)
            return true;

        if (mPipelineDirty || !mPipelineObject)
        {
            String src;
            if (!loadShaderSource(mShaderSource, mShaderPath, src))
                return false;

            String programName = "BPR";
            if (!mShaderPath.isEmpty())
                programName = File::fileNameWithoutExtension(resolveShaderPath(mShaderPath));
            else if (!mShaderSource.isEmpty())
                programName = "material";

            ProgramOptions vsOpt;
            vsOpt.name = (programName + "-vs").cstr();
            vsOpt.source = src.cstr();
            vsOpt.entry = "VSMain";
            vsOpt.type = ProgramType::Vertex;
            vsOpt.target = ProgramTarget::SM_5_0;
            mVS = mDevice->createProgram(vsOpt);

            ProgramOptions psOpt;
            psOpt.name = (programName + "-ps").cstr();
            psOpt.source = src.cstr();
            psOpt.entry = "PSMain";
            psOpt.type = ProgramType::Fragment;
            psOpt.target = ProgramTarget::SM_5_0;
            mPS = mDevice->createProgram(psOpt);

            std::vector<VertexElement> vElements = mVertexElements;
            if (vElements.empty())
            {
                vElements.push_back({"POSITION", 0, 0, Format::R32G32B32_FLOAT});
                vElements.push_back({"NORMAL", 0, 12, Format::R32G32B32_FLOAT});
                vElements.push_back({"COLOR", 0, 24, Format::R32G32B32A32_FLOAT});
                vElements.push_back({"TEXCOORD", 0, 40, Format::R32G32_FLOAT});
            }

            mPipelineObject = mDevice->createPipelineObject();
            mPipelineObject->begin();
            mPipelineObject->setProgram(ProgramType::Vertex, mVS);
            mPipelineObject->setProgram(ProgramType::Fragment, mPS);
            mPipelineObject->setVertexElements(vElements);
            mPipelineObject->setFillMode(mFillMode);
            mPipelineObject->setCullMode(mCullMode);
            mPipelineObject->setDepthStencilState(mDepthStencil);
            for (const auto& sampler : mSamplers)
                mPipelineObject->setSamplerState(sampler.first, sampler.second);
            mPipelineObject->setBlendState(mBlend);
            mPipelineObject->end();

            if (!mDefaultTexture)
            {
                TextureOptions options;
                options.width = 1;
                options.height = 1;
                options.mipCount = 1;
                options.format = Format::R8G8B8A8_UNORM;
                mDefaultTexture = mDevice->createTexture(options);
                mTextureUploaded = false;
            }
        }

        this->bindResources();
        mPipelineDirty = false;
        mBindingsDirty = false;
        return mPipelineObject && mPipelineBindings;
    }

    bool Material::isReady() const
    {
        return mDevice && mPipelineObject && mPipelineBindings && !mPipelineDirty && !mBindingsDirty;
    }

    void Material::bind(CommandBuffer::Ref cmd)
    {
        if (!cmd || !this->isReady())
            return;
        this->uploadDefaultTexture(cmd);
        this->flushUniforms();
        cmd->setPipelineObject(mPipelineObject);
        cmd->setPipelineBindings(mPipelineBindings);
    }
}
