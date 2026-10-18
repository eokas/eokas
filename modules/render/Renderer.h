#ifndef _EOKAS_RENDER_RENDERER_H_
#define _EOKAS_RENDER_RENDERER_H_

#include "./header.h"
#include "./Light.h"
#include "./Material.h"

namespace eokas
{
    struct Space;

    class Renderer
    {
    public:
        using Ref = std::shared_ptr<Renderer>;

        Renderer() = default;
        ~Renderer();

        bool create();
        void destroy();
        bool isAvailable() const { return mDevice != nullptr; }
        Device::Ref device() const { return mDevice; }

        Surface::Ref attach(void* windowHandle, uint32_t width, uint32_t height);
        void detach(Surface::Ref& surface);
        void resize(const Surface::Ref& surface, uint32_t width, uint32_t height);
        void render(const Surface::Ref& surface, Space& space);

    private:
        void ensureResources(Space& space);
        void ensureShadowPass();
        bool findSunLight(const Space& space, Light::Ref& outLight, int& outIndex) const;
        Matrix4 buildShadowMatrix(const Space& space, const Light& light) const;
        void updateLighting(Space& space, const Matrix4* shadowMatrix, int shadowLightIndex);
        void renderShadowPass(CommandBuffer::Ref cmd, Space& space);

        Device::Ref mDevice;
        CommandBuffer::Ref mCommandBuffer;
        RenderTarget::Ref mShadowTarget;
        Material::Ref mShadowMaterial;
        bool mShadowAsShaderResource = false;
        static constexpr uint32_t kShadowMapSize = 2048;
    };
}

#endif
