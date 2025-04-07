#pragma once

// Windows 头文件
#include <Windows.h>
#include <strsafe.h>
#include <comdef.h>
#include <wrl.h>
using namespace Microsoft;
using namespace Microsoft::WRL;

// 必要的 DX 头文件
#include <dxgi1_6.h>
#include <d3d12.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

// C 运行时头文件
#include <string>
#include <exception>
#include <vector>
#include <map>
#include <codecvt>

// libgpu
#include "gpu/main.h"
#include "./Utilities.h"

namespace eokas::gpu {
    
    class Graphics {
        Device::Ref mDevice;
        Viewport mViewport;
        PipelineState::Ref mPipelineState;
        CommandBuffer::Ref mCommandBuffer;
        DynamicBuffer::Ref mVertexBuffer;
        uint32_t vDataLength;
        uint32_t vDataStride;
        DynamicBuffer::Ref mIndexBuffer;
        uint32_t iDataLength;
        Format iDataFormat;
        Texture::Ref mTexture;
    
    public:
        void init(HWND windowHandle, int32_t windowWidth, int32_t windowHeight) {
            mDevice = GPUFactory::createDevice(windowHandle, windowWidth, windowHeight);
            
            mViewport.left = 0;
            mViewport.top = 0;
            mViewport.right = windowWidth;
            mViewport.bottom = windowHeight;
            mViewport.front = 0.01f;
            mViewport.back = 100.0f;
            
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
            
            mPipelineState = mDevice->createPipelineState();
            mPipelineState->begin();
            mPipelineState->setProgram(ProgramType::Vertex, vs);
            mPipelineState->setProgram(ProgramType::Fragment, ps);
            mPipelineState->setVertexElements(vElements);
            mPipelineState->setTexture(0, mTexture);
            mPipelineState->end();
            
            mCommandBuffer = mDevice->createCommandBuffer(mPipelineState);
            
            MeshInfo mesh;
            //MeshFactory::createQuad(mesh, 1.0f, 1.0f);
            //MeshFactory::createBox(mesh, 1.0f, 1.0f, 1.0f);
            //MeshFactory::createSphere(mesh, 0.5f, 30, 30);
            MeshFactory::createCylinder(mesh, 0.5f, 1.0f, 30, 3);
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
            // fill command buffer
            {
                mCommandBuffer->reset(mPipelineState);
                mCommandBuffer->setViewport(mViewport);
                
                RenderTarget::Ref renderTarget = mDevice->getActiveRenderTarget();
                
                Barrier begin;
                begin.resource = renderTarget;
                begin.before = D3D12_RESOURCE_STATE_PRESENT;
                begin.after = D3D12_RESOURCE_STATE_RENDER_TARGET;
                mCommandBuffer->barrier({begin});
                
                mCommandBuffer->setRenderTargets({renderTarget});
                float clearColor[4] = {0.0f, 0.2f, 0.4f, 1.0f};
                mCommandBuffer->clearRenderTarget(renderTarget, clearColor);
                mCommandBuffer->setTopology(Topology::TriangleList);
                mCommandBuffer->setVertexBuffer(mVertexBuffer, vDataLength, vDataStride);
                mCommandBuffer->setIndexBuffer(mIndexBuffer, iDataLength, iDataFormat);
                mCommandBuffer->drawIndexedInstanced(iDataLength / 4, 1, 0, 0, 0);
                
                Barrier end;
                end.resource = renderTarget;
                end.before = D3D12_RESOURCE_STATE_RENDER_TARGET;
                end.after = D3D12_RESOURCE_STATE_PRESENT;
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
