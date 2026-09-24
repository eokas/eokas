#pragma once

#include "gpu/header.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <comdef.h>
#include <wrl.h>
using namespace Microsoft;
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>

#include <d3d12shader.h>
#include <d3dcompiler.h>

#include <dxgidebug.h>
#include <set>

namespace eokas
{
    struct DX12Device;
    struct DX12Surface;
    
    struct DX12Utils
    {
        static DXGI_FORMAT transferFormat(Format format);
        static D3D12_PRIMITIVE_TOPOLOGY transferTopology(Topology topology);
        static D3D12_COMPARISON_FUNC transferCompareOp(CompareOp op);
        static D3D12_RESOURCE_STATES transferResourceState(ResourceState state);
        static D3D12_FILTER transferFilter(SamplerFilterMode minFilter, SamplerFilterMode magFilter, SamplerFilterMode mipFilter);
        static D3D12_TEXTURE_ADDRESS_MODE transferAddressMode(SamplerAddressMode mode);
        static void fillStaticSampler(D3D12_STATIC_SAMPLER_DESC& desc, uint32_t shaderRegister, const SamplerState& state);
        static D3D12_BLEND transferBlendFactor(BlendFactor factor);
        static D3D12_BLEND_OP transferBlendOp(BlendOp op);
    };
    
    struct DX12DescriptorHeap
    {
        enum class Usage
        {
            RTV,
            SRV,
            UAV,
            DSV
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

    struct DX12StaticBuffer : public StaticBuffer
    {
        ComPtr <ID3D12Resource> mResource;
        uint32_t mLength = 0;
        D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COPY_DEST;

        DX12StaticBuffer(const DX12Device& device, uint32_t length);

        virtual void* getNativeResource() const override;
        virtual uint32_t getLength() const override;
    };

    struct DX12MutableBuffer : public MutableBuffer
    {
        ComPtr<ID3D12Resource> mResource;
        uint32_t mLength = 0;

        DX12MutableBuffer(const DX12Device& device, uint32_t length);

        virtual void* getNativeResource() const override;
        virtual uint32_t getLength() const override;
        virtual void* map() override;
        virtual void unmap() override;
    };

    struct DX12Surface : public Surface
    {
        DX12Device& mDevice;
        void* mWindowHandle = nullptr;
        uint32_t mWidth = 0;
        uint32_t mHeight = 0;

        ComPtr <IDXGISwapChain3> mSwapChain;
        uint32_t mFrameBufferIndex = 0;

        std::shared_ptr<DX12DescriptorHeap> mRTVHeap;
        std::shared_ptr<DX12DescriptorHeap> mDSVHeap;
        DX12RenderTarget::Ref mRenderTargets[kFrameCount];
        DX12RenderTarget::Ref mDepthTargets[kFrameCount];

        DX12Surface(DX12Device& device, void* windowHandle, uint32_t windowWidth, uint32_t windowHeight);
        virtual ~DX12Surface();

        void createTargets();
        void detach();

        virtual void* getWindowHandle() const override;
        virtual uint32_t getWidth() const override;
        virtual uint32_t getHeight() const override;
        virtual RenderTarget::Ref getActiveRenderTarget() override;
        virtual RenderTarget::Ref getActiveDepthTarget() override;
        virtual void present() override;
        virtual void resize(uint32_t width, uint32_t height) override;
    };

    struct DX12DynamicBuffer : public DynamicBuffer
    {
        const DX12Device& mDevice;
        ComPtr<ID3D12Resource> mResource;
        uint32_t mLength = 0;
        uint32_t mFrameSize = 0;
        uint32_t mFrameCount = 0;

        DX12DynamicBuffer(const DX12Device& device, uint32_t length);

        virtual void* getNativeResource() const override;
        virtual uint32_t getLength() const override;
        virtual uint32_t getBindOffset() const override;
        virtual void* map() override;
        virtual void unmap() override;
    };
    
    struct DX12Texture : public Texture
    {
        TextureOptions mOptions;
        ComPtr <ID3D12Resource> mResource;
        D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COPY_DEST;
        
        DX12Texture(const DX12Device& device, const TextureOptions& options);
        
        virtual void* getNativeResource() const override;
        virtual const TextureOptions& getOptions() const override;
    };
    
    struct DX12Program : public Program
    {
        ProgramOptions mOptions;
        ComPtr <ID3DBlob> mCode;
        ComPtr <ID3DBlob> mError;
        PipelineLayout mLayout;
        
        DX12Program(const DX12Device& device, const ProgramOptions& options);
        
        virtual const ProgramOptions& getOptions() const override;
        virtual const PipelineLayout& getLayout() const override;
    };
    
    struct DX12PipelineObject : public PipelineObject
    {
        const DX12Device& mDevice;
        
        ComPtr <ID3D12RootSignature> mRootSignature;
        std::vector<std::string> mVertexSemanticNames;
        std::vector<D3D12_INPUT_ELEMENT_DESC> mVertexElements;
        std::map<ProgramType, DX12Program::Ref> mPrograms;
        
        D3D12_FILL_MODE mFillMode = D3D12_FILL_MODE_SOLID;
        D3D12_CULL_MODE mCullMode = D3D12_CULL_MODE_BACK;
        DepthStencilState mDepthStencil;
        bool mDepthStencilSet = false;
        std::map<uint32_t, SamplerState> mSamplers;
        BlendState mBlend;
        
        ComPtr <ID3D12PipelineState> mPipelineState;
        PipelineLayout mLayout;
        std::set<uint32_t> mCBVRegisters;
        std::set<uint32_t> mSRVRegisters;
        std::set<uint32_t> mSamplerRegisters;
        
        DX12PipelineObject(const DX12Device& device);
        
        virtual void begin() override;
        virtual void setVertexElements(std::vector<VertexElement>& vElements) override;
        virtual void setProgram(ProgramType type, Program::Ref program) override;
        virtual void setFillMode(FillMode fillMode) override;
        virtual void setCullMode(CullMode cullMode) override;
        virtual void setDepthStencilState(const DepthStencilState& state) override;
        virtual void setSamplerState(uint32_t index, const SamplerState& state) override;
        virtual void setBlendState(const BlendState& state) override;
        virtual void end() override;
        virtual const PipelineLayout& getLayout() const override;
    };

    struct DX12PipelineBindings : public PipelineBindings
    {
        const DX12Device& mDevice;
        PipelineLayout mLayout;
        std::map<uint32_t, Buffer::Ref> mUniformBuffers;
        std::map<uint32_t, DX12Texture::Ref> mTextures;
        std::shared_ptr<DX12DescriptorHeap> mSRVHeap;

        DX12PipelineBindings(const DX12Device& device, const PipelineLayout& layout);

        virtual const PipelineLayout& getLayout() const override;
        virtual void begin() override;
        virtual void setUniformBufferBySlot(uint32_t slot, Buffer::Ref buffer) override;
        virtual void setUniformBufferByName(const std::string& name, Buffer::Ref buffer) override;
        virtual void setTextureBySlot(uint32_t slot, Texture::Ref texture) override;
        virtual void setTextureByName(const std::string& name, Texture::Ref texture) override;
        virtual void end() override;
    };
    
    struct DX12CommandBuffer : public CommandBuffer
    {
        const DX12Device& mDevice;
        ComPtr <ID3D12GraphicsCommandList> mCommandList;
        
        std::vector<ComPtr<ID3D12Resource>> mUploadResources;
        PipelineObject::Ref mCurrentPipeline;
        
        DX12CommandBuffer(const DX12Device& device);
        
        virtual void reset() override;
        virtual void setPipelineObject(PipelineObject::Ref pipeline) override;
        virtual void setPipelineBindings(PipelineBindings::Ref bindings) override;
        virtual void setRenderTargets(const std::vector<RenderTarget::Ref>& renderTargets, RenderTarget::Ref depthStencil) override;
        virtual void clearRenderTarget(RenderTarget::Ref renderTarget, float(& color)[4]) override;
        virtual void clearDepthStencil(RenderTarget::Ref depthStencil, float depth, uint32_t stencil) override;
        virtual void setViewport(const Viewport& viewport) override;
        virtual void setTopology(Topology topology) override;
        virtual void setVertexBuffer(Buffer::Ref buffer, uint32_t length, uint32_t stride) override;
        virtual void setIndexBuffer(Buffer::Ref buffer, uint32_t length, Format format) override;
        virtual void drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) override;
        virtual void fillBuffer(StaticBuffer::Ref target, const void* data, uint32_t size) override;
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
        ComPtr <ID3D12CommandAllocator> mCommandAllocators[kFrameCount];
        
        ComPtr <ID3D12Fence> mFence;
        UINT64 mFenceValues[kFrameCount];
        HANDLE mFenceEvent = nullptr;
        uint32_t mFrameIndex = 0;
        std::vector<DX12Surface*> mSurfaces;
        
        DX12Device();
        virtual ~DX12Device();
        
        virtual uint32_t getFrameIndex() const override;
        virtual StaticBuffer::Ref createStaticBuffer(uint32_t length) override;
        virtual MutableBuffer::Ref createMutableBuffer(uint32_t length) override;
        virtual DynamicBuffer::Ref createDynamicBuffer(uint32_t length) override;
        virtual Texture::Ref createTexture(const TextureOptions& options) override;
        virtual Program::Ref createProgram(const ProgramOptions& options) override;
        virtual PipelineObject::Ref createPipelineObject() override;
        virtual PipelineBindings::Ref createPipelineBindings(const PipelineLayout& layout) override;
        virtual PipelineBindings::Ref createPipelineBindings(PipelineObject::Ref pipeline) override;
        virtual CommandBuffer::Ref createCommandBuffer() override;
        virtual void commitCommandBuffer(CommandBuffer::Ref commandBuffer) override;
        virtual void waitForGPU() override;
        virtual void waitForNextFrame() override;
        virtual Surface::Ref createSurface(void* windowHandle, uint32_t windowWidth, uint32_t windowHeight) override;
    };
}

