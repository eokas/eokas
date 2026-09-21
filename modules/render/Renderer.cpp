#include "./Renderer.h"
#include "./Space.h"
#include <cstring>

namespace eokas
{
    Renderer::~Renderer()
    {
        destroy();
    }

    bool Renderer::create(void* windowHandle, uint32_t width, uint32_t height)
    {
        destroy();
        mDevice = GPUFactory::createDevice(windowHandle, width, height);
        return mDevice != nullptr;
    }

    void Renderer::destroy()
    {
        if (mDevice)
            mDevice->waitForGPU();
        mCommandBuffer.reset();
        mDevice.reset();
    }

    void Renderer::ensureResources(Space& space)
    {
        if (!space.lightingUniforms)
            space.lightingUniforms = mDevice->createDynamicBuffer(sizeof(LightingUniforms));
        for (auto& p : space.primitives)
        {
            if (!p) continue;
            if (!p->objectUniforms)
                p->createResources(mDevice);
            if (p->material)
            {
                p->material->setDevice(mDevice);
                p->material->setUniformBuffer("Lighting", space.lightingUniforms);
                p->material->build();
            }
        }
    }

    void Renderer::updateLighting(Space& space)
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
        {
            void* ptr = space.lightingUniforms->map();
            memcpy(ptr, &data, sizeof(data));
            space.lightingUniforms->unmap();
        }
    }

    void Renderer::render(Space& space)
    {
        if (!mDevice)
            return;

        mDevice->waitForGPU();

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

        updateLighting(space);
        for (auto& p : space.primitives)
        {
            if (!p || !p->visible || !p->ready())
                continue;
            p->updateUniforms(*cam);
        }

        if (!mCommandBuffer)
            mCommandBuffer = mDevice->createCommandBuffer();
        else
            mCommandBuffer->reset();

        mCommandBuffer->setViewport(cam->viewport);
        RenderTarget::Ref rt = mDevice->getActiveRenderTarget();
        RenderTarget::Ref ds = mDevice->getActiveDepthTarget();
        Barrier begin{rt, ResourceState::Present, ResourceState::RenderTarget};
        mCommandBuffer->barrier({begin});
        mCommandBuffer->setRenderTargets({rt}, ds);
        float c[4] = {space.clearColor.r, space.clearColor.g, space.clearColor.b, space.clearColor.a};
        mCommandBuffer->clearRenderTarget(rt, c);
        mCommandBuffer->clearDepthStencil(ds);

        for (auto& p : space.primitives)
        {
            if (!p || !p->visible || !p->ready())
                continue;
            p->material->setUniformBuffer("Transform", p->objectUniforms);
            p->material->bind(mCommandBuffer);
            p->encode(mCommandBuffer);
        }

        Barrier end{rt, ResourceState::RenderTarget, ResourceState::Present};
        mCommandBuffer->barrier({end});
        mCommandBuffer->finish();
        mDevice->commitCommandBuffer(mCommandBuffer);
        mDevice->present();
        mDevice->waitForNextFrame();
    }
}
