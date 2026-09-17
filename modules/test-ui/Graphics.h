#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <string>
#include <exception>
#include <vector>
#include <memory>

#include "ui/main.h"
#include "./Utilities.h"

namespace eokas::ui {

    class Graphics {
        Device::Ref mDevice;
        Viewport mViewport;
        PipelineObject::Ref mPipelineObject;
        PipelineBindings::Ref mPipelineBindings;
        CommandBuffer::Ref mCommandBuffer;
        Texture::Ref mAtlas;
        UIShape mShape;
        UIFont mFont;
        UILayout mRoot;
        float mWindowWidth = 800.0f;
        float mWindowHeight = 600.0f;

    public:
        void init(HWND windowHandle, int32_t windowWidth, int32_t windowHeight) {
            mWindowWidth = (float)windowWidth;
            mWindowHeight = (float)windowHeight;
            mDevice = GPUFactory::createDevice(windowHandle, windowWidth, windowHeight);

            mViewport.left = 0;
            mViewport.top = 0;
            mViewport.right = windowWidth;
            mViewport.bottom = windowHeight;
            mViewport.front = 0.0f;
            mViewport.back = 1.0f;

            const char* hlsl = "../shaders/004-ui.hlsl";
            auto vs = this->compileShader(hlsl, ProgramType::Vertex, ProgramTarget::SM_5_0, "VSMain");
            auto ps = this->compileShader(hlsl, ProgramType::Fragment, ProgramTarget::SM_5_0, "PSMain");

            std::vector<VertexElement> vElements;
            vElements.push_back({"POSITION", 0, 0, Format::R32G32_FLOAT});
            vElements.push_back({"TEXCOORD", 0, 8, Format::R32G32_FLOAT});
            vElements.push_back({"COLOR", 0, 16, Format::R32G32B32A32_FLOAT});

            const char* fontPath = "../modules/test-ui/fonts/Roboto-Regular.ttf";
            if (!mFont.open(fontPath, 24))
            {
                throw std::runtime_error("Failed to load UI font.");
            }

            TextureOptions options;
            options.width = mFont.atlasSize();
            options.height = mFont.atlasSize();
            options.mipCount = 1;
            options.format = Format::R8G8B8A8_UNORM;
            mAtlas = mDevice->createTexture(options);

            mShape.create(mDevice);
            mShape.setTexture(mAtlas);

            mPipelineObject = mDevice->createPipelineObject();
            mPipelineObject->begin();
            mPipelineObject->setProgram(ProgramType::Vertex, vs);
            mPipelineObject->setProgram(ProgramType::Fragment, ps);
            mPipelineObject->setVertexElements(vElements);
            mPipelineObject->setCullMode(CullMode::None);
            DepthStencilState depthStencil;
            depthStencil.depthTest = false;
            depthStencil.depthWrite = false;
            depthStencil.depthFunc = CompareOp::Always;
            mPipelineObject->setDepthStencilState(depthStencil);
            SamplerState sampler;
            sampler.minFilter = SamplerFilterMode::Linear;
            sampler.magFilter = SamplerFilterMode::Linear;
            sampler.mipFilter = SamplerFilterMode::Point;
            sampler.addressU = SamplerAddressMode::Clamp;
            sampler.addressV = SamplerAddressMode::Clamp;
            sampler.addressW = SamplerAddressMode::Clamp;
            mPipelineObject->setSamplerState(0, sampler);
            BlendState blend;
            blend.enabled = true;
            blend.srcColor = BlendFactor::SrcAlpha;
            blend.dstColor = BlendFactor::OneMinusSrcAlpha;
            blend.colorOp = BlendOp::Add;
            blend.srcAlpha = BlendFactor::One;
            blend.dstAlpha = BlendFactor::OneMinusSrcAlpha;
            blend.alphaOp = BlendOp::Add;
            mPipelineObject->setBlendState(blend);
            mPipelineObject->end();

            mPipelineBindings = mDevice->createPipelineBindings(mPipelineObject);
            mPipelineBindings->begin();
            mPipelineBindings->setUniformBufferByName("Transform", mShape.uniformBuffer);
            mPipelineBindings->setTextureByName("gMainTexture", mAtlas);
            mPipelineBindings->end();

            mCommandBuffer = mDevice->createCommandBuffer(mPipelineBindings);

            mCommandBuffer->fillTexture(mAtlas, mFont.atlasRgba());
            mCommandBuffer->finish();
            mDevice->commitCommandBuffer(mCommandBuffer);
            mDevice->waitForGPU();

            mRoot.rect = Rect(40.0f, 40.0f, 400.0f, 360.0f);
            mRoot.direction = UILayoutDirection::Vertical;
            mRoot.padding = 16.0f;
            mRoot.spacing = 12.0f;
            mRoot.color = Color(0.2f, 0.2f, 0.25f, 0.8f);

            auto image = std::make_shared<UIImage>();
            image->rect = Rect(0.0f, 0.0f, 240.0f, 240.0f);
            image->uv = Rect(0.0f, 0.0f, 1.0f, 1.0f);
            image->color = Color(1.0f, 1.0f, 1.0f, 1.0f);
            mRoot.addChild(image);

            auto text = std::make_shared<UIText>();
            text->rect = Rect(0.0f, 0.0f, 360.0f, 32.0f);
            text->text = "Hello, Eokas UI \x7F";
            text->fontSize = 24.0f;
            text->font = &mFont;
            text->color = Color(1.0f, 0.92f, 0.4f, 1.0f);
            mRoot.addChild(text);
        }

        void quit() {
        }

        void tick(float delta) {
            (void)delta;

            Matrix4 proj = Matrix4::IDENTITY;
            proj.value[0][0] = 2.0f / mWindowWidth;
            proj.value[1][1] = -2.0f / mWindowHeight;
            proj.value[3][0] = -1.0f;
            proj.value[3][1] = 1.0f;
            mShape.setProjection(proj);

            mShape.begin();
            mRoot.render(mShape);
            mShape.end();

            mCommandBuffer->reset(mPipelineBindings);
            mCommandBuffer->setViewport(mViewport);

            RenderTarget::Ref renderTarget = mDevice->getActiveRenderTarget();
            RenderTarget::Ref depthTarget = mDevice->getActiveDepthTarget();

            Barrier begin;
            begin.resource = renderTarget;
            begin.before = ResourceState::Present;
            begin.after = ResourceState::RenderTarget;
            mCommandBuffer->barrier({begin});

            mCommandBuffer->setRenderTargets({renderTarget}, depthTarget);
            float clearColor[4] = {0.12f, 0.12f, 0.16f, 1.0f};
            mCommandBuffer->clearRenderTarget(renderTarget, clearColor);
            mCommandBuffer->clearDepthStencil(depthTarget);
            mCommandBuffer->setTopology(Topology::TriangleList);
            mCommandBuffer->setVertexBuffer(mShape.vertexBuffer, mShape.vertexLength, mShape.vertexStride);
            mCommandBuffer->setIndexBuffer(mShape.indexBuffer, mShape.indexLength, mShape.indexFormat);
            mCommandBuffer->drawIndexedInstanced(mShape.indexCount, 1, 0, 0, 0);

            Barrier end;
            end.resource = renderTarget;
            end.before = ResourceState::RenderTarget;
            end.after = ResourceState::Present;
            mCommandBuffer->barrier({end});

            mCommandBuffer->finish();

            mDevice->commitCommandBuffer(mCommandBuffer);
            mDevice->present();
            mDevice->waitForNextFrame();
        }

        Program::Ref compileShader(const std::string& file, ProgramType type, ProgramTarget target, const char* entry) {
            ProgramOptions options;
            options.name = file;
            options.source = Utilities::readTextFile(file);
            options.type = type;
            options.target = target;
            options.entry = entry;
            return mDevice->createProgram(options);
        }
    };
}
