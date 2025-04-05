
#include "./dx12.h"

#include <strsafe.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <exception>
#include <vector>
#include <map>
#include <codecvt>

namespace eokas
{
    class HRException : public std::exception
    {
    public:
        HRException(HRESULT hr, LPCSTR file, UINT line)
            : std::exception(), m_error(hr)
        {
#ifdef UNICODE
            const wchar_t* wcs = m_error.ErrorMessage();
            std::wstring_convert<std::codecvt_utf8<wchar_t>> convertor;
            std::string mbs = convertor.to_bytes(wcs);
#else
            std::string mbs = m_error.ErrorMessage();
#endif
            char buffer[1024] = {0};
            sprintf_s(buffer, "\n  at %s : %d", file, line);
            m_what = mbs + buffer;
        }
        
        const char* what() const noexcept override
        {
            return m_what.c_str();
        }
    
    private:
        _com_error m_error;
        std::string m_what;
    };

#define _ThrowIfFailed(hr) { HRESULT ret = (hr); if(FAILED(ret)) throw HRException(ret, __FILE__, __LINE__); }
    
    DXGI_FORMAT DX12Utils::transferFormat(Format format)
    {
        switch (format)
        {
            case Format::Unknown:
                break;
            case Format::R32_UINT: return DXGI_FORMAT_R32_UINT;
            case Format::R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
            case Format::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
            case Format::R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case Format::R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
            case Format::R32G32B32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
            case Format::R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case Format::D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
            case Format::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case Format::D16_UNORM: return DXGI_FORMAT_D16_UNORM;
        }
        return DXGI_FORMAT_UNKNOWN;
    }
    
    D3D12_PRIMITIVE_TOPOLOGY DX12Utils::transferTopology(Topology topology)
    {
        switch(topology)
        {
            case Topology::Undefined: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
            case Topology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
            case Topology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            case Topology::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
            case Topology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case Topology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            case Topology::LineList_Adj: return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
            case Topology::LineStrip_Adj: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
            case Topology::TriangleList_Adj: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
            case Topology::TriangleStrip_Adj: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
            default:
                break;
        }
        return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
    
    DX12DescriptorHeap::DX12DescriptorHeap(const DX12Device& device, Usage usage, u32_t numDescriptorsPerHeap)
        : mDevice(device)
        , mDesc()
    {
        auto& dxDevice = device.mDevice;
        
        if(usage == Usage::RTV)
        {
            mDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            mDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        }
        else if(usage == Usage::SRV || usage == Usage::UAV)
        {
            mDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            mDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        }
        mDesc.NumDescriptors = numDescriptorsPerHeap;
        
        mHeaps.clear();
        mDescriptorStride = dxDevice->GetDescriptorHandleIncrementSize(mDesc.Type);
        mDescriptorCount = 0;
    }
    
    DX12DescriptorHeap::~DX12DescriptorHeap()
    {
        for(const auto& heap : mHeaps)
        {
            if(heap != nullptr)
            {
                heap->Release();
            }
        }
        mHeaps.clear();
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::acquire()
    {
        auto& dxDevice = mDevice.mDevice;
        
        u32_t heapIndex = mDescriptorCount / mDesc.NumDescriptors;
        u32_t heapOffset = mDescriptorCount % mDesc.NumDescriptors;
        
        ComPtr<ID3D12DescriptorHeap> heap;
        if(heapIndex < mHeaps.size())
        {
            heap = mHeaps[heapIndex];
        }
        else
        {
            _ThrowIfFailed(dxDevice->CreateDescriptorHeap(&mDesc, IID_PPV_ARGS(&heap)));
            mHeaps.push_back(heap);
        }
        
        D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += mDescriptorStride * heapOffset;
        
        mDescriptorCount += 1;
        
        return handle;
    }
    
    void* DX12RenderTarget::getNativeResource() const
    {
        return mResource.Get();
    }
    
    DX12DynamicBuffer::DX12DynamicBuffer(const DX12Device& device, uint32_t length, uint32_t usage)
    {
        auto& mDevice = device.mDevice;
        
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Alignment = 0;
        bufferDesc.Width = length;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.SampleDesc.Quality = 0;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        
        D3D12_HEAP_PROPERTIES bufferHeapProps = {};
        bufferHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        
        _ThrowIfFailed(mDevice->CreateCommittedResource(
            &bufferHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mResource)));
    }
    
    void* DX12DynamicBuffer::getNativeResource() const
    {
        return mResource.Get();
    }
    
    void* DX12DynamicBuffer::map()
    {
        UINT8* ptr = nullptr;
        _ThrowIfFailed(mResource->Map(0, nullptr, (void**) &ptr));
        return ptr;
    }
    
    void DX12DynamicBuffer::unmap()
    {
        mResource->Unmap(0, nullptr);
    }
    
    DX12Texture::DX12Texture(const DX12Device& device, const TextureOptions& options)
        : mOptions(options)
    {
        auto& dxDevice = device.mDevice;
        
        // 1. 准备纹理数据和描述符
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Alignment = 0;
        textureDesc.Width = options.width;
        textureDesc.Height = options.height;
        textureDesc.DepthOrArraySize = options.depth == 0 ? 1 : options.depth;
        textureDesc.MipLevels = options.mipCount;
        textureDesc.Format = DX12Utils::transferFormat(options.format);
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        
        // 2. 创建纹理资源
        D3D12_HEAP_PROPERTIES textureHeapProps = {};
        textureHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        _ThrowIfFailed(dxDevice->CreateCommittedResource(&textureHeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mResource)));
    }
    
    void* DX12Texture::getNativeResource() const
    {
        return mResource.Get();
    }
    
    const TextureOptions& DX12Texture::getOptions() const
    {
        return this->mOptions;
    }
    
    DX12Program::DX12Program(const DX12Device& device, const ProgramOptions& options)
        : mOptions(options)
    {
        
        UINT compileFlags = 0;
#if defined(_DEBUG)
        compileFlags |= D3DCOMPILE_DEBUG;
        compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        
        std::string target = "";
        {
            if (options.type == ProgramType::Vertex) target += "vs";
            else if (options.type == ProgramType::Fragment) target += "ps";
            else if (options.type == ProgramType::Compute) target += "cs";
            
            if (options.target == ProgramTarget::SM_3_0) target += "_3_0";
            else if (options.target == ProgramTarget::SM_3_1) target += "_3_1";
            else if (options.target == ProgramTarget::SM_4_5) target += "_4_5";
            else if (options.target == ProgramTarget::SM_5_0) target += "_5_0";
            else if (options.target == ProgramTarget::SM_6_0) target += "_6_0";
            else if (options.target == ProgramTarget::SM_6_8) target += "_6_8";
        }
        
        _ThrowIfFailed(D3DCompile(options.source.c_str(), options.source.size(), options.name.c_str(), nullptr, nullptr, options.entry.c_str(), target.c_str(), compileFlags, 0, &mCode, &mError));
        
        if (mError != nullptr)
        {
            std::string str((const char*) mError->GetBufferPointer(), mError->GetBufferSize());
            throw std::runtime_error(str.c_str());
        }
    }
    
    const ProgramOptions& DX12Program::getOptions() const
    {
        return mOptions;
    }
    
    uint32_t DX12Program::getTextureCount() const
    {
        return 0;
    }
    
    DX12PipelineState::DX12PipelineState(const DX12Device& device)
        : mDevice(device)
    {
    }
    
    void DX12PipelineState::begin()
    {
    
    }
    
    void DX12PipelineState::setVertexElements(std::vector<VertexElement>& vElements)
    {
        mVertexElements.clear();
        for (size_t index = 0; index < vElements.size(); index++)
        {
            const VertexElement& ve = vElements.at(index);
            
            mVertexElements.push_back({ve.semanticName.c_str(), ve.semanticIndex, DX12Utils::transferFormat(ve.format), 0, ve.offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
        }
    }
    
    void DX12PipelineState::setProgram(ProgramType type, Program::Ref program)
    {
        mPrograms[type] = program;
    }
    
    void DX12PipelineState::setTexture(uint32_t index, Texture::Ref texture)
    {
        mTextures[index] = texture;
    }
    
    void DX12PipelineState::setFillMode(FillMode fillMode)
    {
        switch(fillMode)
        {
            case FillMode::Solid: mFillMode = D3D12_FILL_MODE_SOLID; break;
            case FillMode::Wireframe: mFillMode = D3D12_FILL_MODE_WIREFRAME; break;
        }
    }
    
    void DX12PipelineState::setCullMode(CullMode cullMode)
    {
        switch(cullMode)
        {
            case CullMode::None: mCullMode = D3D12_CULL_MODE_NONE; break;
            case CullMode::Front: mCullMode = D3D12_CULL_MODE_FRONT; break;
            case CullMode::Back: mCullMode = D3D12_CULL_MODE_BACK; break;
        }
    }
    
    void DX12PipelineState::end()
    {
        auto& dxDevice = mDevice.mDevice;
        
        // Create Root Signature
        {
            D3D12_STATIC_SAMPLER_DESC samplers[1] = {{}};
            samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            samplers[0].MipLODBias = 0;
            samplers[0].MinLOD = 0.0f;
            samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
            samplers[0].MaxAnisotropy = 0;
            samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            samplers[0].ShaderRegister = 0;
            samplers[0].RegisterSpace = 0;
            samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            
            D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
            if (FAILED(dxDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
            {
                featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }
            
            D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.Version = featureData.HighestVersion;
            if (rootSignatureDesc.Version == D3D_ROOT_SIGNATURE_VERSION_1_1)
            {
                D3D12_DESCRIPTOR_RANGE1 descriptorRanges[1] = {};
                descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                descriptorRanges[0].NumDescriptors = 1;
                descriptorRanges[0].BaseShaderRegister = 0;
                descriptorRanges[0].RegisterSpace = 0;
                descriptorRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
                
                D3D12_ROOT_PARAMETER1 parameters[1] = {};
                parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                parameters[0].DescriptorTable.NumDescriptorRanges = 1;
                parameters[0].DescriptorTable.pDescriptorRanges = (const D3D12_DESCRIPTOR_RANGE1*) descriptorRanges;
                
                rootSignatureDesc.Desc_1_1.NumParameters = 1;
                rootSignatureDesc.Desc_1_1.pParameters = parameters;
                rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
                rootSignatureDesc.Desc_1_1.pStaticSamplers = samplers;
                rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            }
            else
            {
                D3D12_DESCRIPTOR_RANGE descriptorRanges[1] = {};
                descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                descriptorRanges[0].NumDescriptors = 1;
                descriptorRanges[0].BaseShaderRegister = 0;
                descriptorRanges[0].RegisterSpace = 0;
                
                D3D12_ROOT_PARAMETER parameters[1] = {};
                parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                parameters[0].DescriptorTable.NumDescriptorRanges = 1;
                parameters[0].DescriptorTable.pDescriptorRanges = (const D3D12_DESCRIPTOR_RANGE*) descriptorRanges;
                
                rootSignatureDesc.Desc_1_0.NumParameters = 1;
                rootSignatureDesc.Desc_1_0.pParameters = parameters;
                rootSignatureDesc.Desc_1_0.NumStaticSamplers = 1;
                rootSignatureDesc.Desc_1_0.pStaticSamplers = samplers;
                rootSignatureDesc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            }
            
            ComPtr<ID3DBlob> rootSignatureBlob;
            ComPtr<ID3DBlob> errorBlob;
            _ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &rootSignatureBlob, &errorBlob));
            _ThrowIfFailed(dxDevice->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
        }
        
        // Create Pipeline State
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.InputLayout.pInputElementDescs = mVertexElements.data();
            psoDesc.InputLayout.NumElements = (UINT) mVertexElements.size();
            
            psoDesc.pRootSignature = mRootSignature.Get();
            if (mPrograms.find(ProgramType::Vertex) != mPrograms.end())
            {
                ComPtr<ID3DBlob> code = dynamic_cast<DX12Program*>(mPrograms[ProgramType::Vertex].get())->mCode;
                psoDesc.VS = {code->GetBufferPointer(), code->GetBufferSize()};
            }
            if (mPrograms.find(ProgramType::Fragment) != mPrograms.end())
            {
                ComPtr<ID3DBlob> code = dynamic_cast<DX12Program*>(mPrograms[ProgramType::Fragment].get())->mCode;
                psoDesc.PS = {code->GetBufferPointer(), code->GetBufferSize()};
            }
            
            psoDesc.RasterizerState.FillMode = mFillMode;
            psoDesc.RasterizerState.CullMode = mCullMode;
            
            psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
            psoDesc.BlendState.IndependentBlendEnable = FALSE;
            psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            
            psoDesc.DepthStencilState.DepthEnable = FALSE;
            psoDesc.DepthStencilState.StencilEnable = FALSE;
            
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.SampleDesc.Count = 1;
            
            _ThrowIfFailed(dxDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState)));
        }
        
        // Create SRV Heap
        {
            mSRVHeap = std::make_shared<DX12DescriptorHeap>(
                mDevice,
                DX12DescriptorHeap::Usage::SRV,
                mTextures.size());
            
            for (auto& node: mTextures)
            {
                auto index = node.first;
                auto texture = node.second;
                auto dxResource = dynamic_cast<DX12Texture*>(texture.get())->mResource;
                
                // 确定 SRV 描述符堆的偏移量
                D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mSRVHeap->acquire();
                
                // 创建 SRV 描述符并将其绑定到纹理资源
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DX12Utils::transferFormat(texture->getOptions().format);
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MipLevels = 1;
                dxDevice->CreateShaderResourceView(dxResource.Get(), &srvDesc, srvHandle);
            }
        }
    }
    
    DX12CommandBuffer::DX12CommandBuffer(const DX12Device& device, const DX12PipelineState& pso)
        : mDevice(device)
    {
        
        auto& dxDevice = device.mDevice;
        auto& dxPSO = pso.mPipelineState;
        auto& dxCommandAllocator = device.mCommandAllocators[device.mFrameBufferIndex];
        
        _ThrowIfFailed(dxDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, dxCommandAllocator.Get(), dxPSO.Get(), IID_PPV_ARGS(&mCommandList)));
        
        //_ThrowIfFailed(mCommandList->Close());
    }
    
    void DX12CommandBuffer::reset(PipelineState::Ref pso)
    {
        auto& dxCommandAllocator = mDevice.mCommandAllocators[mDevice.mFrameBufferIndex];
        auto* pipelineState = dynamic_cast<DX12PipelineState*>(pso.get());
        
        auto& dxPipelineState = pipelineState->mPipelineState;
        _ThrowIfFailed(dxCommandAllocator->Reset());
        _ThrowIfFailed(mCommandList->Reset(dxCommandAllocator.Get(), dxPipelineState.Get()));
        
        auto& dxRootSignature = pipelineState->mRootSignature;
        mCommandList->SetGraphicsRootSignature(dxRootSignature.Get());
        
        std::vector<ID3D12DescriptorHeap*> dxSRVHeapList(pipelineState->mSRVHeap->mHeaps.size());
        for(size_t i = 0; i < dxSRVHeapList.size(); i++)
        {
            dxSRVHeapList[i] = pipelineState->mSRVHeap->mHeaps[i].Get();
        }
        mCommandList->SetDescriptorHeaps(dxSRVHeapList.size(), dxSRVHeapList.data());
        for(size_t i = 0; i < dxSRVHeapList.size(); i++)
        {
            mCommandList->SetGraphicsRootDescriptorTable(i, dxSRVHeapList[i]->GetGPUDescriptorHandleForHeapStart());
        }
    }
    
    void DX12CommandBuffer::setRenderTargets(const std::vector<RenderTarget::Ref>& renderTargets)
    {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv;
        for (size_t i = 0; i < renderTargets.size(); i++)
        {
            auto dxRT = dynamic_cast<DX12RenderTarget*>(renderTargets.at(i).get());
            rtv.push_back(dxRT->mView);
        }
        mCommandList->OMSetRenderTargets((UINT) rtv.size(), rtv.data(), FALSE, nullptr);
    }
    
    void DX12CommandBuffer::clearRenderTarget(RenderTarget::Ref renderTarget, float(& color)[4])
    {
        auto dxRT = dynamic_cast<DX12RenderTarget*>(renderTarget.get());
        mCommandList->ClearRenderTargetView(dxRT->mView, color, 0, nullptr);
    }
    
    void DX12CommandBuffer::setViewport(const Viewport& viewport)
    {
        D3D12_VIEWPORT dxViewport;
        dxViewport.TopLeftX = viewport.left;
        dxViewport.TopLeftY = viewport.top;
        dxViewport.Width = viewport.right - viewport.left;
        dxViewport.Height = viewport.bottom - viewport.top;
        dxViewport.MinDepth = viewport.front;
        dxViewport.MaxDepth = viewport.back;
        
        D3D12_RECT dxScissorRect;
        dxScissorRect.left = (LONG) viewport.left;
        dxScissorRect.top = (LONG) viewport.top;
        dxScissorRect.right = (LONG) viewport.right;
        dxScissorRect.bottom = (LONG) viewport.bottom;
        
        mCommandList->RSSetViewports(1, &dxViewport);
        mCommandList->RSSetScissorRects(1, &dxScissorRect);
    }
    
    void DX12CommandBuffer::setTopology(Topology topology)
    {
        D3D_PRIMITIVE_TOPOLOGY dxTopology = DX12Utils::transferTopology(topology);
        mCommandList->IASetPrimitiveTopology(dxTopology);
    }
    
    void DX12CommandBuffer::setVertexBuffer(DynamicBuffer::Ref buffer, uint32_t length, uint32_t stride)
    {
        DX12DynamicBuffer* dxVBO = dynamic_cast<DX12DynamicBuffer*>(buffer.get());
        const auto& dxVBR = dxVBO->mResource;
        
        D3D12_VERTEX_BUFFER_VIEW dxVBV;
        dxVBV.BufferLocation = dxVBR->GetGPUVirtualAddress();
        dxVBV.SizeInBytes = length;
        dxVBV.StrideInBytes = stride;
        
        mCommandList->IASetVertexBuffers(0, 1, &dxVBV);
    }
    
    void DX12CommandBuffer::setIndexBuffer(DynamicBuffer::Ref buffer, uint32_t length, Format format)
    {
        DX12DynamicBuffer* dxIBO = dynamic_cast<DX12DynamicBuffer*>(buffer.get());
        const auto& dxIBR = dxIBO->mResource;
        
        D3D12_INDEX_BUFFER_VIEW dxIBV;
        dxIBV.BufferLocation = dxIBR->GetGPUVirtualAddress();
        dxIBV.SizeInBytes = length;
        dxIBV.Format = DX12Utils::transferFormat(format);
        
        mCommandList->IASetIndexBuffer(&dxIBV);
    }
    
    void DX12CommandBuffer::drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation)
    {
        
        mCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    }
    
    void DX12CommandBuffer::barrier(const std::vector<Barrier>& barriers)
    {
        std::vector<D3D12_RESOURCE_BARRIER> dxBarriers;
        dxBarriers.resize(barriers.size());
        for (size_t index = 0; index < barriers.size(); index++)
        {
            const Barrier& barrier = barriers.at(index);
            
            D3D12_RESOURCE_BARRIER& dxBarrier = dxBarriers.at(index);
            dxBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            dxBarrier.Transition.pResource = (ID3D12Resource*) barrier.resource->getNativeResource();
            dxBarrier.Transition.Subresource = 0;
            dxBarrier.Transition.StateBefore = (D3D12_RESOURCE_STATES) barrier.before;
            dxBarrier.Transition.StateAfter = (D3D12_RESOURCE_STATES) barrier.after;
        }
        mCommandList->ResourceBarrier((UINT) dxBarriers.size(), dxBarriers.data());
    }
    
    void DX12CommandBuffer::finish()
    {
        _ThrowIfFailed(mCommandList->Close());
    }
    
    void DX12CommandBuffer::fillTexture(Texture::Ref target, const std::vector<uint8_t>& source)
    {
        auto& dxDevice = mDevice.mDevice;
        
        DX12Texture* dxTarget = dynamic_cast<DX12Texture*>(target.get());
        
        D3D12_RESOURCE_DESC dxTargetDesc = dxTarget->mResource->GetDesc();
        
        // 1. 获取目标贴图的信息
        UINT subresourceIndex = 0;
        UINT subresourceCount = 1;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT uploadFootprint;
        UINT64 uploadBufferSize;
        UINT numRows;
        UINT64 rowSizeInBytes;
        dxDevice->GetCopyableFootprints(&dxTargetDesc, subresourceIndex, subresourceCount, 0, &uploadFootprint, &numRows, &rowSizeInBytes, &uploadBufferSize);
        
        // 2. 创建上传堆
        
        {
            D3D12_RESOURCE_DESC bufferDesc = {};
            bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            bufferDesc.Alignment = 0;
            bufferDesc.Width = uploadBufferSize;
            bufferDesc.Height = 1;
            bufferDesc.DepthOrArraySize = 1;
            bufferDesc.MipLevels = 1;
            bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
            bufferDesc.SampleDesc.Count = 1;
            bufferDesc.SampleDesc.Quality = 0;
            bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            
            D3D12_HEAP_PROPERTIES bufferHeapProps = {};
            bufferHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            _ThrowIfFailed(dxDevice->CreateCommittedResource(&bufferHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)));
        }
        
        // 3. 将数据从 std::vector<UINT8> 复制到上传堆中
        const UINT8* srcPtr = source.data();
        UINT8* dstPtr;
        _ThrowIfFailed(uploadBuffer->Map(0, nullptr, (void**) &dstPtr));
        for (UINT row = 0; row < numRows; ++row)
        {
            memcpy(dstPtr, srcPtr, rowSizeInBytes);
            dstPtr += uploadFootprint.Footprint.RowPitch;
            srcPtr += rowSizeInBytes;
        }
        uploadBuffer->Unmap(0, nullptr);
        
        // 4. 将数据从上传堆复制到纹理资源中
        {
            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = uploadBuffer.Get();
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint = uploadFootprint;
            
            D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
            dstLocation.pResource = dxTarget->mResource.Get();
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = 0;
            
            mCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
            
            D3D12_RESOURCE_BARRIER onFinish = {};
            onFinish.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            onFinish.Transition.pResource = dxTarget->mResource.Get();
            onFinish.Transition.Subresource = 0;
            onFinish.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            onFinish.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            mCommandList->ResourceBarrier(1, &onFinish);
        }
    }
    
    DX12Device::DX12Device(void* windowHandle, uint32_t windowWidth, uint32_t windowHeight)
    {
        UINT dxgiFactoryFlags = 0U;

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ComPtr<ID3D12Debug> dc;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dc))))
            {
                if (SUCCEEDED(dc->QueryInterface(IID_PPV_ARGS(&mDebugController))))
                {
                    mDebugController->EnableDebugLayer();
                    mDebugController->SetEnableGPUBasedValidation(true);
                }
                
                // Enable additional debug layers.
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif
        
        // Create DXGI Factory
        _ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mDXGIFactory)));
        _ThrowIfFailed(mDXGIFactory->MakeWindowAssociation((HWND) windowHandle, DXGI_MWA_NO_ALT_ENTER));
        
        // Enum Adapter and Create Device
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != mDXGIFactory->EnumAdapters1(adapterIndex, &mDXGIAdapter); adapterIndex++)
        {
            DXGI_ADAPTER_DESC1 desc = {};
            mDXGIAdapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }
            
            _ThrowIfFailed(D3D12CreateDevice(mDXGIAdapter.Get(), mFeatureLevel, IID_PPV_ARGS(&mDevice)));
#if defined(_DEBUG)
            _ThrowIfFailed(mDevice->QueryInterface(IID_PPV_ARGS(&mDebugDevice)));
#endif
            break;
        }
        if (mDevice == nullptr)
        {
            throw std::runtime_error("Create Device Failed.");
        }
        
        // Create Command Queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        _ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
        
        // Create Swap Chain
        ComPtr<IDXGISwapChain> swapchain;
        DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
        swapchainDesc.BufferCount = kFrameBufferCount;
        swapchainDesc.BufferDesc.Width = windowWidth;
        swapchainDesc.BufferDesc.Height = windowHeight;
        swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.OutputWindow = (HWND) windowHandle;
        swapchainDesc.Windowed = TRUE;
        _ThrowIfFailed(mDXGIFactory->CreateSwapChain(mCommandQueue.Get(), &swapchainDesc, &swapchain));
        _ThrowIfFailed(swapchain.As(&mSwapChain));
        mFrameBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
        
        // Create RTV Heap
        u32_t numHandlePerHeap = max(kFrameBufferCount, 8);
        mRTVHeap = std::make_shared<DX12DescriptorHeap>(*this, DX12DescriptorHeap::Usage::RTV, numHandlePerHeap);
        
        // Get RenderTarget and Create RTV
        for (UINT i = 0; i < kFrameBufferCount; i++)
        {
            mRenderTargets[i] = std::make_shared<DX12RenderTarget>();
            DX12RenderTarget* dxRT = dynamic_cast<DX12RenderTarget*>(mRenderTargets[i].get());
            _ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&dxRT->mResource)));
            dxRT->mView = mRTVHeap->acquire();
            mDevice->CreateRenderTargetView(dxRT->mResource.Get(), nullptr, dxRT->mView);
        }
        
        // Create CommandAllocators
        for (UINT i = 0; i < kFrameBufferCount; i++)
        {
            _ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocators[i])));
        }
        
        // Create Fence
        memset(mFenceValues, 0, sizeof(UINT64) * kFrameBufferCount);
        _ThrowIfFailed(mDevice->CreateFence(mFenceValues[mFrameBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
        mFenceValues[mFrameBufferIndex]++;
        mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (mFenceEvent == nullptr)
        {
            _ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    
    DX12Device::~DX12Device()
    {
        this->waitForGPU();
        CloseHandle(mFenceEvent);
        mFenceEvent = nullptr;
    }
    
    RenderTarget::Ref DX12Device::getActiveRenderTarget()
    {
        return mRenderTargets[mFrameBufferIndex];
    }
    
    DynamicBuffer::Ref DX12Device::createDynamicBuffer(uint32_t length, uint32_t usage)
    {
        return std::make_shared<DX12DynamicBuffer>(*this, length, usage);
    }
    
    Texture::Ref DX12Device::createTexture(const TextureOptions& options)
    {
        return std::make_shared<DX12Texture>(*this, options);
    }
    
    Program::Ref DX12Device::createProgram(const ProgramOptions& options)
    {
        return std::make_shared<DX12Program>(*this, options);
    }
    
    PipelineState::Ref DX12Device::createPipelineState()
    {
        return std::make_shared<DX12PipelineState>(*this);
    }
    
    CommandBuffer::Ref DX12Device::createCommandBuffer(const PipelineState::Ref pso)
    {
        const DX12PipelineState& mPSO = dynamic_cast<const DX12PipelineState&>(*pso.get());
        return std::make_shared<DX12CommandBuffer>(*this, mPSO);
    }
    
    void DX12Device::commitCommandBuffer(const CommandBuffer::Ref commandBuffer)
    {
        const DX12CommandBuffer* dxCommandBuffer = dynamic_cast<const DX12CommandBuffer*>(commandBuffer.get());
        ID3D12CommandList* commandLists[] = {dxCommandBuffer->mCommandList.Get()};
        mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
    }
    
    void DX12Device::present()
    {
        HRESULT hr = mSwapChain->Present(1, 0);
        if (FAILED(hr))
        {
            _ThrowIfFailed(mDevice->GetDeviceRemovedReason());
        }
    }
    
    void DX12Device::waitForGPU()
    {
        const UINT64 currentFenceValue = mFenceValues[mFrameBufferIndex];
        
        _ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), currentFenceValue));
        
        _ThrowIfFailed(mFence->SetEventOnCompletion(currentFenceValue, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
        
        mFenceValues[mFrameBufferIndex] = currentFenceValue + 1;
    }
    
    void DX12Device::waitForNextFrame()
    {
        const UINT64 currentFenceValue = mFenceValues[mFrameBufferIndex];
        
        _ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), currentFenceValue));
        
        mFrameBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
        
        const UINT64 completed = mFence->GetCompletedValue();
        if (completed < mFenceValues[mFrameBufferIndex])
        {
            _ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValues[mFrameBufferIndex], mFenceEvent));
            WaitForSingleObject(mFenceEvent, INFINITE);
        }
        
        mFenceValues[mFrameBufferIndex] = currentFenceValue + 1;
    }
    
    Device::Ref GPUFactory::createDevice(void* windowHandle, uint32_t windowWidth, uint32_t windowHeight)
    {
        return std::make_shared<DX12Device>(windowHandle, windowWidth, windowHeight);
    }
}
