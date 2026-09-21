#ifndef _EOKAS_RENDER_MATERIAL_H_
#define _EOKAS_RENDER_MATERIAL_H_

#include "./header.h"

namespace eokas
{
    class Material
    {
    public:
        using Ref = std::shared_ptr<Material>;

        Material();

        void setDevice(Device::Ref device);
        void setShaderPath(const String& path);
        void setShaderSource(const String& source);

        void setVertexElements(const std::vector<VertexElement>& elements);
        void addVertexElement(const VertexElement& element);

        void setFillMode(FillMode mode);
        void setCullMode(CullMode mode);
        void setDepthStencilState(const DepthStencilState& state);
        void setBlendState(const BlendState& state);
        void setSamplerState(uint32_t index, const SamplerState& state);

        bool setParameter(const String& name, float value);
        bool setParameter(const String& name, const Vector2& value);
        bool setParameter(const String& name, const Vector3& value);
        bool setParameter(const String& name, const Vector4& value);
        bool setParameter(const String& name, const Color& value);
        bool setParameter(const String& name, Texture::Ref texture);

        bool getParameter(const String& name, float& out) const;
        bool getParameter(const String& name, Vector2& out) const;
        bool getParameter(const String& name, Vector3& out) const;
        bool getParameter(const String& name, Vector4& out) const;
        bool getParameter(const String& name, Color& out) const;
        bool getParameter(const String& name, Texture::Ref& out) const;

        bool setUniformBuffer(const String& name, DynamicBuffer::Ref buffer);

        bool build();
        void bind(CommandBuffer::Ref cmd);
        bool isReady() const;

    private:
        void invalidatePipeline();
        void invalidateDeviceResources();
        void bindResources();
        void pruneTextures(const PipelineLayout& layout);
        void uploadDefaultTexture(CommandBuffer::Ref cmd);
        void flushUniforms();
        bool hasLayout() const { return mPipelineObject != nullptr; }
        bool isTextureNameValid(const String& name) const;
        bool isUniformBufferNameValid(const String& name) const;

        Device::Ref mDevice;
        String mShaderPath;
        String mShaderSource;
        std::vector<VertexElement> mVertexElements;

        FillMode mFillMode = FillMode::Solid;
        CullMode mCullMode = CullMode::Front;
        DepthStencilState mDepthStencil;
        BlendState mBlend;
        std::vector<std::pair<uint32_t, SamplerState>> mSamplers;

        MaterialUniforms mUniformData{};
        DynamicBuffer::Ref mMaterialParamsBuffer;
        std::vector<std::pair<String, Texture::Ref>> mTextures;
        std::vector<std::pair<String, DynamicBuffer::Ref>> mUniformBuffers;

        Program::Ref mVS;
        Program::Ref mPS;
        PipelineObject::Ref mPipelineObject;
        PipelineBindings::Ref mPipelineBindings;
        Texture::Ref mDefaultTexture;
        bool mPipelineDirty = true;
        bool mBindingsDirty = true;
        bool mUniformsDirty = true;
        bool mTextureUploaded = false;
    };
}

#endif
