#pragma once

// Windows 头文件
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

// C 运行时头文件
#include <string>
#include <exception>
#include <vector>

// libgpu
#include "gpu/main.h"
#include "mesh/GeoMesh.h"
#include "./Utilities.h"

namespace eokas::gpu {
    
    class Graphics {
        Device::Ref mDevice;
        Viewport mViewport;
        PipelineObject::Ref mPipelineObject;
        PipelineBindings::Ref mPipelineBindings;
        CommandBuffer::Ref mCommandBuffer;
        DynamicBuffer::Ref mVertexBuffer;
        uint32_t vDataLength;
        uint32_t vDataStride;
        DynamicBuffer::Ref mIndexBuffer;
        uint32_t iDataLength;
        Format iDataFormat;
        Texture::Ref mTexture;
        DynamicBuffer::Ref mUniformBuffer;
        uint32_t mIndexCount;
        Matrix4 mViewProj;
        float mAngleX = 0.0f;
        float mAngleY = 0.0f;
        float mAngleZ = 0.0f;
        float mSpeedX = 0.0f;
        float mSpeedY = 0.0f;
        float mSpeedZ = 0.0f;
    
    public:
        void init(HWND windowHandle, int32_t windowWidth, int32_t windowHeight) {
            mDevice = GPUFactory::createDevice(windowHandle, windowWidth, windowHeight);
            
            mViewport.left = 0;
            mViewport.top = 0;
            mViewport.right = windowWidth;
            mViewport.bottom = windowHeight;
            mViewport.front = 0.0f;
            mViewport.back = 1.0f;
            
            float aspect = (float)windowWidth / (float)windowHeight;
            Matrix4 view = Matrix4::lookAtLH(Vector3(0.0f, 0.6f, -2.2f), Vector3(0, 0, 0), Vector3(0, 1, 0));
            Matrix4 proj = Matrix4::perspectiveFovLH(Math::PI / 4.0f, aspect, 0.1f, 100.0f);
            mViewProj = Matrix4::transform(view, proj);
            
            mSpeedX = Random::range(-0.8f, 0.8f);
            mSpeedY = Random::range(-1.0f, 1.0f);
            mSpeedZ = Random::range(-0.6f, 0.6f);
            
            // const char* hlsl = "../shaders/002-texture.hlsl";
            const char* hlsl = "../shaders/003-diffuse.hlsl";
            auto vs = this->compileShader(hlsl, ProgramType::Vertex, ProgramTarget::SM_5_0, "VSMain");
            auto ps = this->compileShader(hlsl, ProgramType::Fragment, ProgramTarget::SM_5_0, "PSMain");
            
            std::vector<VertexElement> vElements;
            vElements.push_back({"POSITION", 0, 0, Format::R32G32B32_FLOAT});
            vElements.push_back({"NORMAL", 0, 12, Format::R32G32B32_FLOAT});
            vElements.push_back({"COLOR", 0, 24, Format::R32G32B32A32_FLOAT});
            vElements.push_back({"TEXCOORD", 0, 40, Format::R32G32_FLOAT});
            
            TextureOptions options;
            options.width = 256;
            options.height = 256;
            options.mipCount = 1;
            options.format = Format::R8G8B8A8_UNORM;
            mTexture = mDevice->createTexture(options);
            
            mUniformBuffer = mDevice->createDynamicBuffer(sizeof(Matrix4) * 2, (uint32_t)BufferUsage::UniformBuffer);
            
            mPipelineObject = mDevice->createPipelineObject();
            mPipelineObject->begin();
            mPipelineObject->setProgram(ProgramType::Vertex, vs);
            mPipelineObject->setProgram(ProgramType::Fragment, ps);
            mPipelineObject->setVertexElements(vElements);
            mPipelineObject->setCullMode(CullMode::Front);
            DepthStencilState depthStencil;
            depthStencil.depthTest = true;
            depthStencil.depthWrite = true;
            depthStencil.depthFunc = CompareOp::Less;
            mPipelineObject->setDepthStencilState(depthStencil);
            SamplerState sampler;
            sampler.minFilter = SamplerFilterMode::Linear;
            sampler.magFilter = SamplerFilterMode::Linear;
            sampler.mipFilter = SamplerFilterMode::Linear;
            sampler.addressU = SamplerAddressMode::Clamp;
            sampler.addressV = SamplerAddressMode::Clamp;
            sampler.addressW = SamplerAddressMode::Clamp;
            mPipelineObject->setSamplerState(0, sampler);
            mPipelineObject->end();

            mPipelineBindings = mDevice->createPipelineBindings(mPipelineObject);
            mPipelineBindings->begin();
            mPipelineBindings->setUniformBuffer(0, mUniformBuffer);
            mPipelineBindings->setTexture(0, mTexture);
            mPipelineBindings->end();
            
            mCommandBuffer = mDevice->createCommandBuffer(mPipelineBindings);
            
            GeoMesh mesh;
            GeoMeshFactory::createBox(mesh, 1.0f, 1.0f, 1.0f);
            mIndexCount = (uint32_t)mesh.indices.size();
            // vb
            {
                vDataStride = sizeof(Vertex);
                vDataLength = vDataStride * mesh.vertices.size();
                mVertexBuffer = mDevice->createDynamicBuffer(vDataLength, 0);
                void* ptr = mVertexBuffer->map();
                memcpy(ptr, mesh.vertices.data(), vDataLength);
                mVertexBuffer->unmap();
            }
            // ib
            {
                iDataLength = mesh.indices.size() * sizeof(uint32_t);
                iDataFormat = Format::R32_UINT;
                mIndexBuffer = mDevice->createDynamicBuffer(iDataLength, 0);
                void* ptr = mIndexBuffer->map();
                memcpy(ptr, mesh.indices.data(), iDataLength);
                mIndexBuffer->unmap();
            }
            // Texture
            {
                auto& options = mTexture->getOptions();
                std::vector<uint8_t> image;
                Utilities::createImage(image, options.width, options.height);
                mCommandBuffer->fillTexture(mTexture, image);
            }
            
            mCommandBuffer->finish();
            mDevice->commitCommandBuffer(mCommandBuffer);
            mDevice->waitForGPU();
        }
        
        void quit() {
        
        }
        
        void tick(float delta) {
            float dt = delta > 0.0f ? delta : (1.0f / 60.0f);
            if (dt > 0.1f) dt = 0.1f;
            mAngleX += mSpeedX * dt;
            mAngleY += mSpeedY * dt;
            mAngleZ += mSpeedZ * dt;
            
            Matrix4 rx = Matrix4::rotate(Vector3(1, 0, 0), mAngleX);
            Matrix4 ry = Matrix4::rotate(Vector3(0, 1, 0), mAngleY);
            Matrix4 rz = Matrix4::rotate(Vector3(0, 0, 1), mAngleZ);
            Matrix4 world = Matrix4::transform(rz, Matrix4::transform(ry, rx));
            Matrix4 wvp = Matrix4::transform(world, mViewProj);
            
            void* ptr = mUniformBuffer->map();
            memcpy(ptr, world.value, sizeof(world.value));
            memcpy((uint8_t*)ptr + sizeof(world.value), wvp.value, sizeof(wvp.value));
            mUniformBuffer->unmap();
            
            // fill command buffer
            {
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
                float clearColor[4] = {0.0f, 0.2f, 0.4f, 1.0f};
                mCommandBuffer->clearRenderTarget(renderTarget, clearColor);
                mCommandBuffer->clearDepthStencil(depthTarget);
                mCommandBuffer->setTopology(Topology::TriangleList);
                mCommandBuffer->setVertexBuffer(mVertexBuffer, vDataLength, vDataStride);
                mCommandBuffer->setIndexBuffer(mIndexBuffer, iDataLength, iDataFormat);
                mCommandBuffer->drawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
                
                Barrier end;
                end.resource = renderTarget;
                end.before = ResourceState::RenderTarget;
                end.after = ResourceState::Present;
                mCommandBuffer->barrier({end});
                
                mCommandBuffer->finish();
            }
            
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
