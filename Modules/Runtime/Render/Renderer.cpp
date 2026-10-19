#include "./Renderer.h"
#include "./Space.h"
#include <cmath>
#include <cstring>

namespace
{
    const char* kShadowDepthShader = R"hlsl(
#pragma pack_matrix(row_major)

cbuffer Transform : register(b0)
{
    float4x4 world;
    float4x4 wvp;
    float4 cameraPos;
};

struct VertexIn
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct Varying
{
    float4 position : SV_POSITION;
};

Varying VSMain(VertexIn input)
{
    Varying output;
    output.position = mul(float4(input.position.xyz, 1.0), wvp);
    output.position.z += 0.0007 * output.position.w;
    return output;
}

float4 PSMain(Varying input) : SV_TARGET
{
    return float4(input.position.z, 0.0, 0.0, 1.0);
}
)hlsl";
}

namespace eokas
{
    Renderer::~Renderer()
    {
        destroy();
    }

    bool Renderer::create()
    {
        destroy();
        mDevice = GPUFactory::createDevice();
        return mDevice != nullptr;
    }

    void Renderer::destroy()
    {
        mShadowMaterial.reset();
        mShadowTarget.reset();
        mShadowAsShaderResource = false;
        mCommandBuffer.reset();
        mDevice.reset();
    }

    Surface::Ref Renderer::attach(void* windowHandle, uint32_t width, uint32_t height)
    {
        if (!mDevice)
            return nullptr;
        return mDevice->createSurface(windowHandle, width, height);
    }

    void Renderer::detach(Surface::Ref& surface)
    {
        if (!surface)
            return;
        surface.reset();
    }

    void Renderer::resize(const Surface::Ref& surface, uint32_t width, uint32_t height)
    {
        if (!surface)
            return;
        surface->resize(width, height);
    }

    void Renderer::ensureShadowPass()
    {
        if (!mShadowTarget)
        {
            RenderTargetOptions options;
            options.width = kShadowMapSize;
            options.height = kShadowMapSize;
            options.colorFormats.push_back(Format::R32_FLOAT);
            options.depthStencilFormat = Format::D32_FLOAT;
            mShadowTarget = mDevice->createRenderTarget(options);
            mShadowAsShaderResource = false;
        }

        if (!mShadowMaterial)
        {
            mShadowMaterial = std::make_shared<Material>();
            mShadowMaterial->setShaderSource(kShadowDepthShader);
            BlendState blend;
            blend.enabled = false;
            mShadowMaterial->setBlendState(blend);
            mShadowMaterial->setCullMode(CullMode::Front);
            mShadowMaterial->setColorBufferFormats({ Format::R32_FLOAT });
            DepthStencilState depth;
            depth.depthTest = true;
            depth.depthWrite = true;
            depth.depthFunc = CompareOp::Less;
            mShadowMaterial->setDepthStencilState(depth);
        }

        mShadowMaterial->setDevice(mDevice);
        mShadowMaterial->build();
    }

    bool Renderer::findSunLight(const Space& space, Light::Ref& outLight, int& outIndex) const
    {
        outLight.reset();
        outIndex = -1;
        int gpuIndex = 0;
        for (const auto& light : space.lights)
        {
            if (!light || !light->enabled)
                continue;
            if (gpuIndex >= (int)kMaxSpaceLights)
                break;
            if (light->type == LightType::Directional)
            {
                outLight = light;
                outIndex = gpuIndex;
                return true;
            }
            gpuIndex += 1;
        }
        return false;
    }

    Matrix4 Renderer::buildShadowMatrix(const Space& space, const Light& light) const
    {
        float extent = space.shadowExtent > 1.0f ? space.shadowExtent : 1.0f;
        Vector3 dir = light.direction().normalized();
        Vector3 up = light.transform.up();
        if (fabsf(Vector3::dot(up, dir)) > 0.95f)
            up = fabsf(dir.y) < 0.95f ? Vector3(0, 1, 0) : Vector3(1, 0, 0);

        Vector3 eye = space.shadowCenter - dir * extent;
        Matrix4 view = Matrix4::lookToLH(eye, dir, up);
        Matrix4 proj = Matrix4::orthographicLH(extent, extent, extent * 0.02f, extent * 2.2f);
        return Matrix4::transform(view, proj);
    }

    void Renderer::ensureResources(Space& space)
    {
        if (!space.lightingUniforms)
            space.lightingUniforms = mDevice->createDynamicBuffer(sizeof(LightingUniforms));

        bool anyCaster = false;
        for (const auto& p : space.primitives)
        {
            if (p && p->castShadow)
            {
                anyCaster = true;
                break;
            }
        }
        if (anyCaster && space.shadowExtent > 1.0f)
            ensureShadowPass();

        SamplerState shadowSampler;
        shadowSampler.minFilter = SamplerFilterMode::Point;
        shadowSampler.magFilter = SamplerFilterMode::Point;
        shadowSampler.mipFilter = SamplerFilterMode::Point;
        shadowSampler.addressU = SamplerAddressMode::Clamp;
        shadowSampler.addressV = SamplerAddressMode::Clamp;
        shadowSampler.addressW = SamplerAddressMode::Clamp;

        Texture::Ref shadowTex;
        if (mShadowTarget)
            shadowTex = mShadowTarget->getColorBuffer(0);

        for (auto& p : space.primitives)
        {
            if (!p) continue;
            if (!p->objectUniforms)
                p->createResources(mDevice);
            if (p->castShadow && !p->shadowUniforms)
                p->shadowUniforms = mDevice->createDynamicBuffer(sizeof(TransformUniforms));
            if (p->material)
            {
                p->material->setDevice(mDevice);
                p->material->setUniformBuffer("Lighting", space.lightingUniforms);
                if (p->receiveShadow)
                {
                    p->material->setParameter("receiveShadow", 1.0f);
                    p->material->setSamplerState(1, shadowSampler);
                    if (shadowTex)
                        p->material->setParameter("gShadowMap", shadowTex);
                }
                p->material->build();
            }
        }
    }

    void Renderer::updateLighting(Space& space, const Matrix4* shadowMatrix, int shadowLightIndex)
    {
        LightingUniforms data{};
        data.ambientAndCount[0] = space.ambient.r;
        data.ambientAndCount[1] = space.ambient.g;
        data.ambientAndCount[2] = space.ambient.b;
        uint32_t count = 0;
        for (auto& l : space.lights)
        {
            if (!l || !l->enabled) continue;
            if (count >= kMaxSpaceLights) break;
            l->fillGpu(data.lights[count]);
            count += 1;
        }
        data.ambientAndCount[3] = (float)count;
        if (shadowMatrix && shadowLightIndex >= 0)
        {
            data.shadowMatrix = *shadowMatrix;
            data.shadowParams[0] = 0.0008f;
            data.shadowParams[1] = 1.0f / (float)kShadowMapSize;
            data.shadowParams[2] = 1.0f;
            data.shadowParams[3] = (float)shadowLightIndex;
        }
        else
        {
            data.shadowMatrix = Matrix4::IDENTITY;
            data.shadowParams[2] = 0.0f;
        }
        {
            void* ptr = space.lightingUniforms->map();
            memcpy(ptr, &data, sizeof(data));
            space.lightingUniforms->unmap();
        }
    }

    void Renderer::renderShadowPass(CommandBuffer::Ref cmd, Space& space)
    {
        Texture2D::Ref shadowColor = mShadowTarget->getColorBuffer(0);
        if (mShadowAsShaderResource)
        {
            Barrier toTarget{shadowColor, ResourceState::ShaderResource, ResourceState::RenderTarget};
            cmd->barrier({toTarget});
            mShadowAsShaderResource = false;
        }

        Viewport viewport;
        viewport.left = 0.0f;
        viewport.top = 0.0f;
        viewport.right = (float)kShadowMapSize;
        viewport.bottom = (float)kShadowMapSize;
        viewport.front = 0.0f;
        viewport.back = 1.0f;
        cmd->setViewport(viewport);
        cmd->setRenderTarget(mShadowTarget);
        float clearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        cmd->clearRenderTargetColorBuffer(mShadowTarget, 0, clearColor);
        cmd->clearRenderTargetDepthStencilBuffer(mShadowTarget);

        for (auto& p : space.primitives)
        {
            if (!p || !p->visible || !p->castShadow || !p->shadowUniforms || !p->ready())
                continue;
            mShadowMaterial->setUniformBuffer("Transform", p->shadowUniforms);
            mShadowMaterial->bind(cmd);
            p->encode(cmd);
        }

        Barrier toSample{shadowColor, ResourceState::RenderTarget, ResourceState::ShaderResource};
        cmd->barrier({toSample});
        mShadowAsShaderResource = true;
    }

    void Renderer::render(const Surface::Ref& surface, Space& space)
    {
        if (!mDevice || !surface)
            return;

        Camera::Ref cam = space.activeCamera ? space.activeCamera : (space.cameras.empty() ? nullptr : space.cameras[0]);
        if (!cam)
            return;

        ensureResources(space);
        Primitive::Ref first;
        for (auto& p : space.primitives)
        {
            if (p && p->visible && p->ready())
            {
                first = p;
                break;
            }
        }
        if (!first)
            return;

        Light::Ref sun;
        int sunIndex = -1;
        bool doShadow = mShadowTarget && mShadowMaterial && mShadowMaterial->isReady()
            && findSunLight(space, sun, sunIndex);
        if (doShadow)
        {
            bool hasCaster = false;
            for (const auto& p : space.primitives)
            {
                if (p && p->visible && p->castShadow && p->shadowUniforms && p->ready())
                {
                    hasCaster = true;
                    break;
                }
            }
            doShadow = hasCaster;
        }

        Matrix4 lightViewProj = doShadow ? buildShadowMatrix(space, *sun) : Matrix4::IDENTITY;
        updateLighting(space, doShadow ? &lightViewProj : nullptr, doShadow ? sunIndex : -1);
        for (auto& p : space.primitives)
        {
            if (!p || !p->visible || !p->ready())
                continue;
            p->updateUniforms(*cam);
            if (doShadow && p->castShadow)
                p->updateShadowUniforms(lightViewProj);
        }

        if (!mCommandBuffer)
            mCommandBuffer = mDevice->createCommandBuffer();
        mCommandBuffer->open();

        if (doShadow)
            renderShadowPass(mCommandBuffer, space);

        mCommandBuffer->setViewport(cam->viewport);
        RenderTarget::Ref fb = surface->getActiveRenderTarget();
        Texture2D::Ref color = fb->getColorBuffer(0);
        Barrier begin{color, ResourceState::Present, ResourceState::RenderTarget};
        mCommandBuffer->barrier({begin});
        mCommandBuffer->setRenderTarget(fb);
        float c[4] = {space.clearColor.r, space.clearColor.g, space.clearColor.b, space.clearColor.a};
        mCommandBuffer->clearRenderTargetColorBuffer(fb, 0, c);
        mCommandBuffer->clearRenderTargetDepthStencilBuffer(fb);

        for (auto& p : space.primitives)
        {
            if (!p || !p->visible || !p->ready())
                continue;
            p->material->setUniformBuffer("Transform", p->objectUniforms);
            p->upload(mCommandBuffer);
            p->material->bind(mCommandBuffer);
            p->encode(mCommandBuffer);
        }

        Barrier end{color, ResourceState::RenderTarget, ResourceState::Present};
        mCommandBuffer->barrier({end});
        mCommandBuffer->close();
        mDevice->commitCommandBuffer(mCommandBuffer);
        surface->present();
        mDevice->waitForNextFrame();
    }
}
