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
            
            auto vs = this->compileShader("../shaders/002-texture.hlsl", ProgramType::Vertex, ProgramTarget::SM_5_0, "VSMain");
            auto ps = this->compileShader("../shaders/002-texture.hlsl", ProgramType::Fragment, ProgramTarget::SM_5_0, "PSMain");
            
            std::vector<VertexElement> vElements;
            vElements.push_back({"POSITION", 0, 0, Format::R32G32B32_FLOAT});
            vElements.push_back({"COLOR", 0, 12, Format::R32G32B32A32_FLOAT});
            vElements.push_back({"TEXCOORD", 0, 28, Format::R32G32_FLOAT});
            
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
            
            // Create Vertex Buffer
            {
                float aspect = windowWidth / (windowHeight + 0.0f);
                Vertex vData[] = {{{-0.9f, +0.9f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                                  {{+0.9f, +0.9f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                                  {{+0.9f, -0.9f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
                                  {{-0.9f, -0.9f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}};
                vDataLength = sizeof(vData);
                vDataStride = sizeof(Vertex);
                
                mVertexBuffer = mDevice->createDynamicBuffer(vDataLength, 0);
                void* ptr = mVertexBuffer->map();
                memcpy(ptr, vData, vDataLength);
                mVertexBuffer->unmap();
            }
            
            // Create Index Buffer
            {
                const uint32_t iData[] = {0, 1, 2, 2, 3, 0};
                iDataLength = sizeof(iData);
                iDataFormat = Format::R32_UINT;
                
                mIndexBuffer = mDevice->createDynamicBuffer(iDataLength, 0);
                void* ptr = mIndexBuffer->map();
                memcpy(ptr, iData, iDataLength);
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
                mCommandBuffer->drawIndexedInstanced(6, 1, 0, 0, 0);
                
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
