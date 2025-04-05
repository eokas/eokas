#ifndef _DX12_H_
#define _DX12_H_

#include "gpu/header.h"

#include <comdef.h>
#include <wrl.h>
using namespace Microsoft;
using namespace Microsoft::WRL;

#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>

namespace eokas
{
    struct DX12Device;
    
    const uint32_t kFrameBufferCount = 2;
    
    struct DX12Utils
    {
        static DXGI_FORMAT transferFormat(Format format);
        static D3D12_PRIMITIVE_TOPOLOGY transferTopology(Topology topology);
    };
    
    struct DX12DescriptorHeap
    {
        enum class Usage
        {
            RTV,
            SRV,
            UAV
        };
        
        const DX12Device& mDevice;
        D3D12_DESCRIPTOR_HEAP_DESC mDesc;
        
        std::vector<ComPtr<ID3D12DescriptorHeap>> mHeaps = {};
        u32_t mDescriptorStride = 0;
        u32_t mDescriptorCount = 0;
        
        DX12DescriptorHeap(const DX12Device& device, Usage usage, u32_t numDescriptorsPerHeap);
        virtual ~DX12DescriptorHeap();
        
        D3D12_CPU_DESCRIPTOR_HANDLE acquire();
    };
    
    struct DX12RenderTarget : public RenderTarget
    {
        ComPtr <ID3D12Resource> mResource;
        D3D12_CPU_DESCRIPTOR_HANDLE mView;
        
        virtual void* getNativeResource() const override;
    };
    
    struct DX12DynamicBuffer : public DynamicBuffer
    {
        ComPtr <ID3D12Resource> mResource;

        DX12DynamicBuffer(const DX12Device& device, uint32_t length, uint32_t usage);
        
        virtual void* getNativeResource() const override;
        virtual void* map() override;
        virtual void unmap() override;
    };
    
    struct DX12Texture : public Texture
    {
        TextureOptions mOptions;
        ComPtr <ID3D12Resource> mResource;
        
        DX12Texture(const DX12Device& device, const TextureOptions& options);
        
        virtual void* getNativeResource() const override;
        virtual const TextureOptions& getOptions() const override;
    };
    
    struct DX12Program : public Program
    {
        ProgramOptions mOptions;
        ComPtr <ID3DBlob> mCode;
        ComPtr <ID3DBlob> mError;
        
        DX12Program(const DX12Device& device, const ProgramOptions& options);
        
        virtual const ProgramOptions& getOptions() const override;
        virtual uint32_t getTextureCount() const override;
    };
    
    struct DX12PipelineState : public PipelineState
    {
        const DX12Device& mDevice;
        
        ComPtr <ID3D12RootSignature> mRootSignature;
        std::vector<D3D12_INPUT_ELEMENT_DESC> mVertexElements;
        std::map<ProgramType, DX12Program::Ref> mPrograms;
        std::map<uint32_t, DX12Texture::Ref> mTextures;
        
        std::shared_ptr<DX12DescriptorHeap> mSRVHeap;
        
        D3D12_FILL_MODE mFillMode = D3D12_FILL_MODE_SOLID;
        D3D12_CULL_MODE mCullMode = D3D12_CULL_MODE_BACK;
        
        ComPtr <ID3D12PipelineState> mPipelineState;
        
        DX12PipelineState(const DX12Device& device);
        
        virtual void begin() override;
        virtual void setVertexElements(std::vector<VertexElement>& vElements) override;
        virtual void setProgram(ProgramType type, Program::Ref program) override;
        virtual void setTexture(uint32_t index, Texture::Ref texture) override;
        virtual void setFillMode(FillMode fillMode) override;
        virtual void setCullMode(CullMode cullMode) override;
        virtual void end() override;
    };
    
    struct DX12CommandBuffer : public CommandBuffer
    {
        const DX12Device& mDevice;
        ComPtr <ID3D12GraphicsCommandList> mCommandList;
        
        ComPtr <ID3D12Resource> uploadBuffer;
        
        DX12CommandBuffer(const DX12Device& device, const DX12PipelineState& pso);
        
        virtual void reset(PipelineState::Ref pso) override;
        virtual void setRenderTargets(const std::vector<RenderTarget::Ref>& renderTargets) override;
        virtual void clearRenderTarget(RenderTarget::Ref renderTarget, float(& color)[4]) override;
        virtual void setViewport(const Viewport& viewport) override;
        virtual void setTopology(Topology topology) override;
        virtual void setVertexBuffer(DynamicBuffer::Ref buffer, uint32_t length, uint32_t stride) override;
        virtual void setIndexBuffer(DynamicBuffer::Ref buffer, uint32_t length, Format format) override;
        virtual void drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) override;
        virtual void fillTexture(Texture::Ref target, const std::vector<uint8_t>& source) override;
        virtual void barrier(const std::vector<Barrier>& barriers) override;
        virtual void finish() override;
    };
    
    struct DX12Device : public Device
    {
        ComPtr <ID3D12Debug1> mDebugController;
        D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL_12_0;
        ComPtr <IDXGIFactory7> mDXGIFactory;
        ComPtr <IDXGIAdapter1> mDXGIAdapter;
        ComPtr <ID3D12Device4> mDevice;
        ComPtr <ID3D12DebugDevice> mDebugDevice;
        ComPtr <ID3D12CommandQueue> mCommandQueue;
        ComPtr <IDXGISwapChain3> mSwapChain;
        uint32_t mFrameBufferIndex;
        
        std::shared_ptr<DX12DescriptorHeap> mRTVHeap;
        DX12RenderTarget::Ref mRenderTargets[kFrameBufferCount];
        ComPtr <ID3D12CommandAllocator> mCommandAllocators[kFrameBufferCount];
        
        ComPtr <ID3D12Fence> mFence;
        UINT64 mFenceValues[kFrameBufferCount];
        HANDLE mFenceEvent;
        
        DX12Device(void* windowHandle, uint32_t windowWidth, uint32_t windowHeight);
        virtual ~DX12Device();
        
        virtual RenderTarget::Ref getActiveRenderTarget() override;
        virtual DynamicBuffer::Ref createDynamicBuffer(uint32_t length, uint32_t usage) override;
        virtual Texture::Ref createTexture(const TextureOptions& options) override;
        virtual Program::Ref createProgram(const ProgramOptions& options) override;
        virtual PipelineState::Ref createPipelineState() override;
        virtual CommandBuffer::Ref createCommandBuffer(PipelineState::Ref pso) override;
        virtual void commitCommandBuffer(CommandBuffer::Ref commandBuffer) override;
        virtual void present() override;
        virtual void waitForGPU() override;
        virtual void waitForNextFrame() override;
    };
}

#endif
