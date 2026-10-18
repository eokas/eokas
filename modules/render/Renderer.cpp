#include "./Renderer.h"
#include "./Space.h"
#include <cstring>

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

        updateLighting(space);
        for (auto& p : space.primitives)
        {
            if (!p || !p->visible || !p->ready())
                continue;
            p->updateUniforms(*cam);
        }

        if (!mCommandBuffer)
            mCommandBuffer = mDevice->createCommandBuffer();
        mCommandBuffer->open();

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
