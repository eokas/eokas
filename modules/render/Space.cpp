#include "./Space.h"
#include <algorithm>
#include <cstring>

namespace eokas
{
    void Space::add(Camera::Ref camera) { if (camera) cameras.push_back(camera); }
    void Space::add(Light::Ref light) { if (light) lights.push_back(light); }
    void Space::add(Primitive::Ref primitive) { if (primitive) primitives.push_back(primitive); }

    void Space::remove(Camera::Ref camera)
    {
        cameras.erase(std::remove(cameras.begin(), cameras.end(), camera), cameras.end());
        if (activeCamera == camera) activeCamera = nullptr;
    }
    void Space::remove(Light::Ref light)
    {
        lights.erase(std::remove(lights.begin(), lights.end(), light), lights.end());
    }
    void Space::remove(Primitive::Ref primitive)
    {
        primitives.erase(std::remove(primitives.begin(), primitives.end(), primitive), primitives.end());
    }

    void Space::ensureResources(Device::Ref device)
    {
        if (!lightingUniforms)
            lightingUniforms = device->createDynamicBuffer(sizeof(LightingUniforms), (uint32_t)BufferUsage::UniformBuffer);
        for (auto& p : primitives)
        {
            if (!p) continue;
            if (p->material && !p->material->built)
                p->material->build(device, lightingUniforms);
            if (!p->ready())
                p->createResources(device);
        }
    }

    void Space::updateLighting()
    {
        LightingUniforms data{};
        data.ambientAndCount[0] = ambient.r;
        data.ambientAndCount[1] = ambient.g;
        data.ambientAndCount[2] = ambient.b;
        uint32_t count = 0;
        for (auto& l : lights)
        {
            if (!l || !l->enabled) continue;
            if (count >= kMaxSpaceLights) break;
            l->fillGpu(data.lights[count]);
            count += 1;
        }
        data.ambientAndCount[3] = (float)count;
        {
            void* ptr = lightingUniforms->map();
            memcpy(ptr, &data, sizeof(data));
            lightingUniforms->unmap();
        }
    }

    void Space::render(Device::Ref device)
    {
        Camera::Ref cam = activeCamera ? activeCamera : (cameras.empty() ? nullptr : cameras[0]);
        if (!device || !cam)
            return;

        ensureResources(device);
        Primitive::Ref first;
        for (auto& p : primitives)
        {
            if (p && p->visible && p->ready())
            {
                first = p;
                break;
            }
        }
        if (!first)
            return;

        updateLighting();
        for (auto& p : primitives)
        {
            if (!p || !p->visible || !p->ready())
                continue;
            p->updateUniforms(*cam);
            if (p->material)
                p->material->updateUniforms();
        }

        if (!commandBuffer)
            commandBuffer = device->createCommandBuffer();
        else
            commandBuffer->reset();

        commandBuffer->setPipelineObject(first->material->pipelineObject);

        for (auto& p : primitives)
        {
            if (p && p->visible && p->ready() && p->material)
                p->material->uploadDefaultTexture(commandBuffer);
        }

        commandBuffer->setViewport(cam->viewport);
        RenderTarget::Ref rt = device->getActiveRenderTarget();
        RenderTarget::Ref ds = device->getActiveDepthTarget();
        Barrier begin{rt, ResourceState::Present, ResourceState::RenderTarget};
        commandBuffer->barrier({begin});
        commandBuffer->setRenderTargets({rt}, ds);
        float c[4] = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
        commandBuffer->clearRenderTarget(rt, c);
        commandBuffer->clearDepthStencil(ds);

        Material::Ref lastMat;
        for (auto& p : primitives)
        {
            if (!p || !p->visible || !p->ready())
                continue;
            if (p->material != lastMat)
            {
                commandBuffer->setPipelineObject(p->material->pipelineObject);
                lastMat = p->material;
            }
            p->material->pipelineBindings->setUniformBufferByName("Transform", p->objectUniforms);
            commandBuffer->setPipelineBindings(p->material->pipelineBindings);
            p->encode(commandBuffer);
        }

        Barrier end{rt, ResourceState::RenderTarget, ResourceState::Present};
        commandBuffer->barrier({end});
        commandBuffer->finish();
        device->commitCommandBuffer(commandBuffer);
        device->present();
        device->waitForNextFrame();
    }
}
