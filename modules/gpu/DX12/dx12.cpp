
#include "./dx12.h"

#include <strsafe.h>

#include <cstdlib>
#include <malloc.h>
#include <memory.h>
#include <string>
#include <exception>
#include <stdexcept>
#include <vector>
#include <map>
#include <codecvt>
#include <d3d11shader.h>

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

    namespace
    {
        constexpr uint32_t kConstantBufferAlignment = 256;

        uint32_t alignUp(uint32_t value, uint32_t alignment)
        {
            return (value + alignment - 1) & ~(alignment - 1);
        }
    }

    static void addReflectedBinding(PipelineLayout& layout, const char* name, D3D_SHADER_INPUT_TYPE type, uint32_t bindPoint, uint32_t bindCount)
    {
        PipelineLayoutEntry entry;
        entry.name = name ? name : "";
        entry.slot = bindPoint;
        entry.count = bindCount;
        if (type == D3D_SIT_CBUFFER)
            entry.type = PipelineResourceType::UniformBuffer;
        else if (type == D3D_SIT_TEXTURE)
            entry.type = PipelineResourceType::Texture;
        else if (type == D3D_SIT_SAMPLER)
            entry.type = PipelineResourceType::Sampler;
        else
            return;
        layout.add(entry);
    }

    static void reflectProgramParameters(ID3DBlob* code, PipelineLayout& parameters)
    {
        ComPtr<ID3D12ShaderReflection> reflector12;
        if (SUCCEEDED(D3DReflect(code->GetBufferPointer(), code->GetBufferSize(), IID_PPV_ARGS(&reflector12))))
        {
            D3D12_SHADER_DESC shaderDesc = {};
            _ThrowIfFailed(reflector12->GetDesc(&shaderDesc));
            for (UINT i = 0; i < shaderDesc.BoundResources; i++)
            {
                D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
                _ThrowIfFailed(reflector12->GetResourceBindingDesc(i, &bindDesc));
                addReflectedBinding(parameters, bindDesc.Name, bindDesc.Type, bindDesc.BindPoint, bindDesc.BindCount);
            }
            return;
        }

        ComPtr<ID3D11ShaderReflection> reflector11;
        _ThrowIfFailed(D3DReflect(
            code->GetBufferPointer(),
            code->GetBufferSize(),
            IID_ID3D11ShaderReflection,
            &reflector11));
        D3D11_SHADER_DESC shaderDesc = {};
        _ThrowIfFailed(reflector11->GetDesc(&shaderDesc));
        for (UINT i = 0; i < shaderDesc.BoundResources; i++)
        {
            D3D11_SHADER_INPUT_BIND_DESC bindDesc = {};
            _ThrowIfFailed(reflector11->GetResourceBindingDesc(i, &bindDesc));
            addReflectedBinding(parameters, bindDesc.Name, bindDesc.Type, bindDesc.BindPoint, bindDesc.BindCount);
        }
    }

    
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

    D3D12_COMPARISON_FUNC DX12Utils::transferCompareOp(CompareOp op)
    {
        switch (op)
        {
            case CompareOp::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
            case CompareOp::Never: return D3D12_COMPARISON_FUNC_NEVER;
            case CompareOp::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
            case CompareOp::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
            case CompareOp::Less: return D3D12_COMPARISON_FUNC_LESS;
            case CompareOp::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case CompareOp::Greater: return D3D12_COMPARISON_FUNC_GREATER;
            case CompareOp::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        }
        return D3D12_COMPARISON_FUNC_LESS;
    }

    D3D12_RESOURCE_STATES DX12Utils::transferResourceState(ResourceState state)
    {
        switch (state)
        {
            case ResourceState::Common: return D3D12_RESOURCE_STATE_COMMON;
            case ResourceState::Present: return D3D12_RESOURCE_STATE_PRESENT;
            case ResourceState::RenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;
            case ResourceState::DepthWrite: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            case ResourceState::DepthRead: return D3D12_RESOURCE_STATE_DEPTH_READ;
            case ResourceState::ShaderResource: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            case ResourceState::CopyDest: return D3D12_RESOURCE_STATE_COPY_DEST;
            case ResourceState::CopySource: return D3D12_RESOURCE_STATE_COPY_SOURCE;
        }
        return D3D12_RESOURCE_STATE_COMMON;
    }

    D3D12_FILTER DX12Utils::transferFilter(SamplerFilterMode minFilter, SamplerFilterMode magFilter, SamplerFilterMode mipFilter)
    {
        if (minFilter == SamplerFilterMode::Anisotropic
            || magFilter == SamplerFilterMode::Anisotropic
            || mipFilter == SamplerFilterMode::Anisotropic)
        {
            return D3D12_FILTER_ANISOTROPIC;
        }
        D3D12_FILTER_TYPE minType = (minFilter == SamplerFilterMode::Point)
            ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR;
        D3D12_FILTER_TYPE magType = (magFilter == SamplerFilterMode::Point)
            ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR;
        D3D12_FILTER_TYPE mipType = (mipFilter == SamplerFilterMode::Point)
            ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR;
        return D3D12_ENCODE_BASIC_FILTER(minType, magType, mipType, D3D12_FILTER_REDUCTION_TYPE_STANDARD);
    }

    D3D12_TEXTURE_ADDRESS_MODE DX12Utils::transferAddressMode(SamplerAddressMode mode)
    {
        switch (mode)
        {
            case SamplerAddressMode::Clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            case SamplerAddressMode::Border: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            case SamplerAddressMode::Repeat: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            case SamplerAddressMode::Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        }
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    }

    void DX12Utils::fillStaticSampler(D3D12_STATIC_SAMPLER_DESC& desc, uint32_t shaderRegister, const SamplerState& state)
    {
        const bool anisotropic =
            state.minFilter == SamplerFilterMode::Anisotropic
            || state.magFilter == SamplerFilterMode::Anisotropic
            || state.mipFilter == SamplerFilterMode::Anisotropic;

        desc = {};
        desc.Filter = transferFilter(state.minFilter, state.magFilter, state.mipFilter);
        desc.AddressU = transferAddressMode(state.addressU);
        desc.AddressV = transferAddressMode(state.addressV);
        desc.AddressW = transferAddressMode(state.addressW);
        desc.MipLODBias = 0;
        desc.MaxAnisotropy = anisotropic ? 16u : 1u;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        desc.MinLOD = 0.0f;
        desc.MaxLOD = D3D12_FLOAT32_MAX;
        desc.ShaderRegister = shaderRegister;
        desc.RegisterSpace = 0;
        desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    D3D12_BLEND DX12Utils::transferBlendFactor(BlendFactor factor)
    {
        switch (factor)
        {
            case BlendFactor::Zero: return D3D12_BLEND_ZERO;
            case BlendFactor::One: return D3D12_BLEND_ONE;
            case BlendFactor::SrcColor: return D3D12_BLEND_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
            case BlendFactor::DstColor: return D3D12_BLEND_DEST_COLOR;
            case BlendFactor::OneMinusDstColor: return D3D12_BLEND_INV_DEST_COLOR;
            case BlendFactor::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
            case BlendFactor::DstAlpha: return D3D12_BLEND_DEST_ALPHA;
            case BlendFactor::OneMinusDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
        }
        return D3D12_BLEND_ONE;
    }

    D3D12_BLEND_OP DX12Utils::transferBlendOp(BlendOp op)
    {
        switch (op)
        {
            case BlendOp::Add: return D3D12_BLEND_OP_ADD;
            case BlendOp::Subtract: return D3D12_BLEND_OP_SUBTRACT;
            case BlendOp::ReverseSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
            case BlendOp::Min: return D3D12_BLEND_OP_MIN;
            case BlendOp::Max: return D3D12_BLEND_OP_MAX;
        }
        return D3D12_BLEND_OP_ADD;
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
        else if(usage == Usage::DSV)
        {
            mDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
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

    DX12StaticBuffer::DX12StaticBuffer(const DX12Device& device, uint32_t length)
    {
        auto& dxDevice = device.mDevice;
        mLength = length;

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
        bufferHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        _ThrowIfFailed(dxDevice->CreateCommittedResource(
            &bufferHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&mResource)));
    }

    void* DX12StaticBuffer::getNativeResource() const
    {
        return mResource.Get();
    }

    uint32_t DX12StaticBuffer::getLength() const
    {
        return mLength;
    }

    DX12MutableBuffer::DX12MutableBuffer(const DX12Device& device, uint32_t length)
        : mLength(length)
    {
        auto& dxDevice = device.mDevice;

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

        _ThrowIfFailed(dxDevice->CreateCommittedResource(
            &bufferHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mResource)));
    }

    void* DX12MutableBuffer::getNativeResource() const
    {
        return mResource.Get();
    }

    uint32_t DX12MutableBuffer::getLength() const
    {
        return mLength;
    }

    void* DX12MutableBuffer::map()
    {
        UINT8* ptr = nullptr;
        _ThrowIfFailed(mResource->Map(0, nullptr, (void**) &ptr));
        return ptr;
    }

    void DX12MutableBuffer::unmap()
    {
        mResource->Unmap(0, nullptr);
    }

    DX12DynamicBuffer::DX12DynamicBuffer(const DX12Device& device, uint32_t length)
        : mDevice(device)
        , mLength(length)
        , mFrameSize(alignUp(length, kConstantBufferAlignment))
        , mFrameCount(kFrameCount)
    {
        auto& dxDevice = device.mDevice;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Alignment = 0;
        bufferDesc.Width = (uint64_t)mFrameSize * mFrameCount;
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

        _ThrowIfFailed(dxDevice->CreateCommittedResource(
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

    uint32_t DX12DynamicBuffer::getLength() const
    {
        return mLength;
    }

    uint32_t DX12DynamicBuffer::getBindOffset() const
    {
        return mDevice.getFrameIndex() * mFrameSize;
    }

    void* DX12DynamicBuffer::map()
    {
        void* base = nullptr;
        D3D12_RANGE readRange = {0, 0};
        _ThrowIfFailed(mResource->Map(0, &readRange, &base));
        return static_cast<uint8_t*>(base) + getBindOffset();
    }

    void DX12DynamicBuffer::unmap()
    {
        const uint32_t offset = getBindOffset();
        D3D12_RANGE written = {offset, offset + mLength};
        mResource->Unmap(0, &written);
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
        
        HRESULT hr = D3DCompile(
            options.source.c_str(),
            options.source.size(),
            options.name.c_str(),
            nullptr, nullptr,
            options.entry.c_str(),
            target.c_str(),
            compileFlags, 0,
            &mCode, &mError);
        
        if (mError != nullptr)
        {
            std::string str((const char*) mError->GetBufferPointer(), mError->GetBufferSize());
            throw std::runtime_error(str.c_str());
        }
        if (FAILED(hr) || mCode == nullptr)
        {
            throw std::runtime_error("D3DCompile failed.");
        }

        reflectProgramParameters(mCode.Get(), mLayout);
    }
    
    const ProgramOptions& DX12Program::getOptions() const
    {
        return mOptions;
    }
    
    const PipelineLayout& DX12Program::getLayout() const
    {
        return mLayout;
    }
    
    DX12PipelineObject::DX12PipelineObject(const DX12Device& device)
        : mDevice(device)
    {
    }
    
    void DX12PipelineObject::begin()
    {
        mSamplers.clear();
        mLayout.entries.clear();
        mCBVRegisters.clear();
        mSRVRegisters.clear();
        mSamplerRegisters.clear();
    }
    
    void DX12PipelineObject::setVertexElements(std::vector<VertexElement>& vElements)
    {
        mVertexSemanticNames.clear();
        mVertexElements.clear();
        mVertexSemanticNames.reserve(vElements.size());
        mVertexElements.reserve(vElements.size());
        for (size_t index = 0; index < vElements.size(); index++)
        {
            const VertexElement& ve = vElements.at(index);
            mVertexSemanticNames.push_back(ve.semanticName);
            mVertexElements.push_back({
                nullptr,
                ve.semanticIndex,
                DX12Utils::transferFormat(ve.format),
                0,
                ve.offset,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            });
        }
        for (size_t index = 0; index < mVertexElements.size(); index++)
        {
            mVertexElements[index].SemanticName = mVertexSemanticNames[index].c_str();
        }
    }
    
    void DX12PipelineObject::setProgram(ProgramType type, Program::Ref program)
    {
        mPrograms[type] = program;
    }
    
    void DX12PipelineObject::setFillMode(FillMode fillMode)
    {
        switch(fillMode)
        {
            case FillMode::Solid: mFillMode = D3D12_FILL_MODE_SOLID; break;
            case FillMode::Wireframe: mFillMode = D3D12_FILL_MODE_WIREFRAME; break;
        }
    }
    
    void DX12PipelineObject::setCullMode(CullMode cullMode)
    {
        switch(cullMode)
        {
            case CullMode::None: mCullMode = D3D12_CULL_MODE_NONE; break;
            case CullMode::Front: mCullMode = D3D12_CULL_MODE_FRONT; break;
            case CullMode::Back: mCullMode = D3D12_CULL_MODE_BACK; break;
        }
    }

    void DX12PipelineObject::setDepthStencilState(const DepthStencilState& state)
    {
        mDepthStencil = state;
        mDepthStencilSet = true;
    }

    void DX12PipelineObject::setSamplerState(uint32_t index, const SamplerState& state)
    {
        mSamplers[index] = state;
    }

    void DX12PipelineObject::setBlendState(const BlendState& state)
    {
        mBlend = state;
    }
    
    void DX12PipelineObject::end()
    {
        auto& dxDevice = mDevice.mDevice;

        mLayout.entries.clear();
        mCBVRegisters.clear();
        mSRVRegisters.clear();
        mSamplerRegisters.clear();
        for (const auto& node : mPrograms)
        {
            for (const auto& entry : node.second->getLayout().entries)
            {
                mLayout.add(entry);
            }
        }
        for (const auto& entry : mLayout.entries)
        {
            if (entry.type == PipelineResourceType::UniformBuffer) mCBVRegisters.insert(entry.slot);
            else if (entry.type == PipelineResourceType::Texture) mSRVRegisters.insert(entry.slot);
            else if (entry.type == PipelineResourceType::Sampler) mSamplerRegisters.insert(entry.slot);
        }
        
        // Create Root Signature
        {
            std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
            for (uint32_t shaderRegister : mSamplerRegisters)
            {
                D3D12_STATIC_SAMPLER_DESC desc = {};
                auto it = mSamplers.find(shaderRegister);
                const SamplerState& state = (it != mSamplers.end()) ? it->second : SamplerState{};
                DX12Utils::fillStaticSampler(desc, shaderRegister, state);
                samplers.push_back(desc);
            }
            
            D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
            if (FAILED(dxDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
            {
                featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }
            
            D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.Version = featureData.HighestVersion;

            std::vector<D3D12_DESCRIPTOR_RANGE1> descriptorRanges1;
            std::vector<D3D12_ROOT_PARAMETER1> parameters1;
            std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges0;
            std::vector<D3D12_ROOT_PARAMETER> parameters0;

            if (rootSignatureDesc.Version == D3D_ROOT_SIGNATURE_VERSION_1_1)
            {
                for (uint32_t reg : mCBVRegisters)
                {
                    D3D12_ROOT_PARAMETER1 parameter = {};
                    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                    parameter.Descriptor.ShaderRegister = reg;
                    parameter.Descriptor.RegisterSpace = 0;
                    parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
                    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                    parameters1.push_back(parameter);
                }
                if (!mSRVRegisters.empty())
                {
                    D3D12_DESCRIPTOR_RANGE1 range = {};
                    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    range.NumDescriptors = *mSRVRegisters.rbegin() - *mSRVRegisters.begin() + 1;
                    range.BaseShaderRegister = *mSRVRegisters.begin();
                    range.RegisterSpace = 0;
                    range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
                    descriptorRanges1.push_back(range);

                    D3D12_ROOT_PARAMETER1 parameter = {};
                    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    parameter.DescriptorTable.NumDescriptorRanges = 1;
                    parameter.DescriptorTable.pDescriptorRanges = descriptorRanges1.data();
                    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                    parameters1.push_back(parameter);
                }
                
                rootSignatureDesc.Desc_1_1.NumParameters = (UINT)parameters1.size();
                rootSignatureDesc.Desc_1_1.pParameters = parameters1.empty() ? nullptr : parameters1.data();
                rootSignatureDesc.Desc_1_1.NumStaticSamplers = (UINT)samplers.size();
                rootSignatureDesc.Desc_1_1.pStaticSamplers = samplers.empty() ? nullptr : samplers.data();
                rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            }
            else
            {
                for (uint32_t reg : mCBVRegisters)
                {
                    D3D12_ROOT_PARAMETER parameter = {};
                    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                    parameter.Descriptor.ShaderRegister = reg;
                    parameter.Descriptor.RegisterSpace = 0;
                    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                    parameters0.push_back(parameter);
                }
                if (!mSRVRegisters.empty())
                {
                    D3D12_DESCRIPTOR_RANGE range = {};
                    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    range.NumDescriptors = *mSRVRegisters.rbegin() - *mSRVRegisters.begin() + 1;
                    range.BaseShaderRegister = *mSRVRegisters.begin();
                    range.RegisterSpace = 0;
                    descriptorRanges0.push_back(range);

                    D3D12_ROOT_PARAMETER parameter = {};
                    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    parameter.DescriptorTable.NumDescriptorRanges = 1;
                    parameter.DescriptorTable.pDescriptorRanges = descriptorRanges0.data();
                    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                    parameters0.push_back(parameter);
                }
                
                rootSignatureDesc.Desc_1_0.NumParameters = (UINT)parameters0.size();
                rootSignatureDesc.Desc_1_0.pParameters = parameters0.empty() ? nullptr : parameters0.data();
                rootSignatureDesc.Desc_1_0.NumStaticSamplers = (UINT)samplers.size();
                rootSignatureDesc.Desc_1_0.pStaticSamplers = samplers.empty() ? nullptr : samplers.data();
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
            psoDesc.RasterizerState.DepthClipEnable = TRUE;
            
            psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
            psoDesc.BlendState.IndependentBlendEnable = FALSE;
            auto& rt0 = psoDesc.BlendState.RenderTarget[0];
            rt0.BlendEnable = mBlend.enabled ? TRUE : FALSE;
            rt0.LogicOpEnable = FALSE;
            rt0.SrcBlend = DX12Utils::transferBlendFactor(mBlend.srcColor);
            rt0.DestBlend = DX12Utils::transferBlendFactor(mBlend.dstColor);
            rt0.BlendOp = DX12Utils::transferBlendOp(mBlend.colorOp);
            rt0.SrcBlendAlpha = DX12Utils::transferBlendFactor(mBlend.srcAlpha);
            rt0.DestBlendAlpha = DX12Utils::transferBlendFactor(mBlend.dstAlpha);
            rt0.BlendOpAlpha = DX12Utils::transferBlendOp(mBlend.alphaOp);
            rt0.LogicOp = D3D12_LOGIC_OP_NOOP;
            rt0.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            
            const bool depthEnable = mDepthStencilSet && mDepthStencil.depthTest;
            psoDesc.DepthStencilState.DepthEnable = depthEnable ? TRUE : FALSE;
            psoDesc.DepthStencilState.DepthWriteMask = (depthEnable && mDepthStencil.depthWrite)
                ? D3D12_DEPTH_WRITE_MASK_ALL
                : D3D12_DEPTH_WRITE_MASK_ZERO;
            psoDesc.DepthStencilState.DepthFunc = DX12Utils::transferCompareOp(mDepthStencil.depthFunc);
            psoDesc.DepthStencilState.StencilEnable = FALSE;
            
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.DSVFormat = depthEnable
                ? DX12Utils::transferFormat(Format::D32_FLOAT)
                : DXGI_FORMAT_UNKNOWN;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.SampleDesc.Count = 1;
            
            _ThrowIfFailed(dxDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState)));
        }
    }

    const PipelineLayout& DX12PipelineObject::getLayout() const
    {
        return mLayout;
    }
    
    DX12PipelineBindings::DX12PipelineBindings(const DX12Device& device, const PipelineLayout& layout)
        : mDevice(device)
        , mLayout(layout)
    {
    }

    const PipelineLayout& DX12PipelineBindings::getLayout() const
    {
        return mLayout;
    }

    void DX12PipelineBindings::begin()
    {
    }

    void DX12PipelineBindings::setUniformBufferBySlot(uint32_t slot, Buffer::Ref buffer)
    {
        if (!mLayout.findBySlot(PipelineResourceType::UniformBuffer, slot))
        {
            throw std::runtime_error("PipelineBindings: unknown uniform buffer slot.");
        }
        mUniformBuffers[slot] = buffer;
    }

    void DX12PipelineBindings::setUniformBufferByName(const std::string& name, Buffer::Ref buffer)
    {
        const PipelineLayoutEntry* entry = mLayout.findByName(PipelineResourceType::UniformBuffer, name);
        if (!entry)
        {
            throw std::runtime_error("PipelineBindings: unknown uniform buffer name.");
        }
        setUniformBufferBySlot(entry->slot, buffer);
    }

    void DX12PipelineBindings::setTextureBySlot(uint32_t slot, Texture::Ref texture)
    {
        if (!mLayout.findBySlot(PipelineResourceType::Texture, slot))
        {
            throw std::runtime_error("PipelineBindings: unknown texture slot.");
        }
        mTextures[slot] = texture;
    }

    void DX12PipelineBindings::setTextureByName(const std::string& name, Texture::Ref texture)
    {
        const PipelineLayoutEntry* entry = mLayout.findByName(PipelineResourceType::Texture, name);
        if (!entry)
        {
            throw std::runtime_error("PipelineBindings: unknown texture name.");
        }
        setTextureBySlot(entry->slot, texture);
    }

    void DX12PipelineBindings::end()
    {
        auto& dxDevice = mDevice.mDevice;
        std::set<uint32_t> srvRegisters;
        for (const auto& entry : mLayout.entries)
        {
            if (entry.type == PipelineResourceType::Texture)
            {
                srvRegisters.insert(entry.slot);
            }
        }
        if (srvRegisters.empty())
        {
            mSRVHeap.reset();
            return;
        }

        uint32_t base = *srvRegisters.begin();
        uint32_t count = *srvRegisters.rbegin() - base + 1;
        mSRVHeap = std::make_shared<DX12DescriptorHeap>(mDevice, DX12DescriptorHeap::Usage::SRV, count);

        for (uint32_t i = 0; i < count; i++)
        {
            uint32_t reg = base + i;
            D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mSRVHeap->acquire();
            if (srvRegisters.find(reg) == srvRegisters.end())
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MipLevels = 1;
                dxDevice->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
                continue;
            }

            auto it = mTextures.find(reg);
            if (it == mTextures.end() || !it->second)
            {
                throw std::runtime_error("PipelineBindings: missing texture slot.");
            }

            auto texture = it->second;
            auto* dxTexture = dynamic_cast<DX12Texture*>(texture.get());
            if (!dxTexture)
            {
                throw std::runtime_error("PipelineBindings: texture is not a DX12Texture.");
            }
            auto dxResource = dxTexture->mResource;
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DX12Utils::transferFormat(texture->getOptions().format);
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels = 1;
            dxDevice->CreateShaderResourceView(dxResource.Get(), &srvDesc, srvHandle);
        }
    }

    DX12CommandBuffer::DX12CommandBuffer(const DX12Device& device)
        : mDevice(device)
    {
        auto& dxDevice = device.mDevice;
        auto& dxCommandAllocator = device.mCommandAllocators[device.mFrameIndex];
        _ThrowIfFailed(dxDevice->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT, dxCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
    }
    
    void DX12CommandBuffer::reset()
    {
        mUploadResources.clear();
        auto& dxCommandAllocator = mDevice.mCommandAllocators[mDevice.mFrameIndex];
        ID3D12PipelineState* pso = nullptr;
        if (auto* current = dynamic_cast<DX12PipelineObject*>(mCurrentPipeline.get()))
        {
            pso = current->mPipelineState.Get();
        }
        _ThrowIfFailed(mCommandList->Reset(dxCommandAllocator.Get(), pso));
        if (mCurrentPipeline)
        {
            setPipelineObject(mCurrentPipeline);
        }
    }

    void DX12CommandBuffer::setPipelineObject(PipelineObject::Ref pipeline)
    {
        auto* pipelineObject = dynamic_cast<DX12PipelineObject*>(pipeline.get());
        if (!pipelineObject)
        {
            throw std::runtime_error("CommandBuffer: invalid pipeline.");
        }
        mCurrentPipeline = pipeline;
        mCommandList->SetPipelineState(pipelineObject->mPipelineState.Get());
        mCommandList->SetGraphicsRootSignature(pipelineObject->mRootSignature.Get());
    }

    void DX12CommandBuffer::setPipelineBindings(PipelineBindings::Ref bindings)
    {
        auto* pipelineObject = dynamic_cast<DX12PipelineObject*>(mCurrentPipeline.get());
        auto* pipelineBindings = dynamic_cast<DX12PipelineBindings*>(bindings.get());
        if (!pipelineObject || !pipelineBindings)
        {
            throw std::runtime_error("CommandBuffer: setPipelineObject before setPipelineBindings.");
        }
        if (!pipelineBindings->getLayout().compatibleWith(pipelineObject->getLayout()))
        {
            throw std::runtime_error("CommandBuffer: PipelineBindings layout is not compatible with current PipelineObject.");
        }

        uint32_t root = 0;
        for (uint32_t reg : pipelineObject->mCBVRegisters)
        {
            auto ubIt = pipelineBindings->mUniformBuffers.find(reg);
            if (ubIt == pipelineBindings->mUniformBuffers.end() || !ubIt->second)
            {
                throw std::runtime_error("PipelineBindings: missing uniform buffer slot.");
            }
            auto* native = static_cast<ID3D12Resource*>(ubIt->second->getNativeResource());
            if (!native)
            {
                throw std::runtime_error("PipelineBindings: uniform buffer has no native resource.");
            }
            mCommandList->SetGraphicsRootConstantBufferView(
                root, native->GetGPUVirtualAddress() + ubIt->second->getBindOffset());
            root += 1;
        }

        if (!pipelineObject->mSRVRegisters.empty())
        {
            if (pipelineBindings->mSRVHeap == nullptr || pipelineBindings->mSRVHeap->mHeaps.empty())
            {
                throw std::runtime_error("PipelineBindings: missing SRV heap.");
            }
            std::vector<ID3D12DescriptorHeap*> dxSRVHeapList(pipelineBindings->mSRVHeap->mHeaps.size());
            for (size_t i = 0; i < dxSRVHeapList.size(); i++)
            {
                dxSRVHeapList[i] = pipelineBindings->mSRVHeap->mHeaps[i].Get();
            }
            mCommandList->SetDescriptorHeaps((UINT)dxSRVHeapList.size(), dxSRVHeapList.data());
            mCommandList->SetGraphicsRootDescriptorTable(
                (UINT)pipelineObject->mCBVRegisters.size(),
                dxSRVHeapList[0]->GetGPUDescriptorHandleForHeapStart());
        }
    }
    
    void DX12CommandBuffer::setRenderTargets(const std::vector<RenderTarget::Ref>& renderTargets, RenderTarget::Ref depthStencil)
    {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv;
        for (size_t i = 0; i < renderTargets.size(); i++)
        {
            auto dxRT = dynamic_cast<DX12RenderTarget*>(renderTargets.at(i).get());
            rtv.push_back(dxRT->mView);
        }
        D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = nullptr;
        if (depthStencil)
        {
            auto dxDS = dynamic_cast<DX12RenderTarget*>(depthStencil.get());
            pDSV = &dxDS->mView;
        }
        mCommandList->OMSetRenderTargets((UINT) rtv.size(), rtv.data(), FALSE, pDSV);
    }
    
    void DX12CommandBuffer::clearRenderTarget(RenderTarget::Ref renderTarget, float(& color)[4])
    {
        auto dxRT = dynamic_cast<DX12RenderTarget*>(renderTarget.get());
        mCommandList->ClearRenderTargetView(dxRT->mView, color, 0, nullptr);
    }

    void DX12CommandBuffer::clearDepthStencil(RenderTarget::Ref depthStencil, float depth, uint32_t stencil)
    {
        auto dxDS = dynamic_cast<DX12RenderTarget*>(depthStencil.get());
        mCommandList->ClearDepthStencilView(
            dxDS->mView,
            D3D12_CLEAR_FLAG_DEPTH,
            depth,
            (UINT8)stencil,
            0,
            nullptr);
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
    
    void DX12CommandBuffer::setVertexBuffer(Buffer::Ref buffer, uint32_t length, uint32_t stride)
    {
        auto* dxRes = static_cast<ID3D12Resource*>(buffer->getNativeResource());
        D3D12_VERTEX_BUFFER_VIEW dxVBV;
        dxVBV.BufferLocation = dxRes->GetGPUVirtualAddress() + buffer->getBindOffset();
        dxVBV.SizeInBytes = length;
        dxVBV.StrideInBytes = stride;
        
        mCommandList->IASetVertexBuffers(0, 1, &dxVBV);
    }
    
    void DX12CommandBuffer::setIndexBuffer(Buffer::Ref buffer, uint32_t length, Format format)
    {
        auto* dxRes = static_cast<ID3D12Resource*>(buffer->getNativeResource());
        D3D12_INDEX_BUFFER_VIEW dxIBV;
        dxIBV.BufferLocation = dxRes->GetGPUVirtualAddress() + buffer->getBindOffset();
        dxIBV.SizeInBytes = length;
        dxIBV.Format = DX12Utils::transferFormat(format);
        
        mCommandList->IASetIndexBuffer(&dxIBV);
    }

    void DX12CommandBuffer::fillBuffer(StaticBuffer::Ref target, const void* data, uint32_t size)
    {
        if (!target || !data || size == 0)
        {
            throw std::runtime_error("CommandBuffer: fillBuffer requires a valid target and data.");
        }

        auto* dxTarget = dynamic_cast<DX12StaticBuffer*>(target.get());
        if (!dxTarget || !dxTarget->mResource)
        {
            throw std::runtime_error("CommandBuffer: fillBuffer target is not a DX12StaticBuffer.");
        }
        if (size > dxTarget->mLength)
        {
            throw std::runtime_error("CommandBuffer: fillBuffer source is larger than the static buffer.");
        }

        auto& dxDevice = mDevice.mDevice;
        ComPtr<ID3D12Resource> upload;
        {
            D3D12_RESOURCE_DESC bufferDesc = {};
            bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            bufferDesc.Alignment = 0;
            bufferDesc.Width = size;
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
            _ThrowIfFailed(dxDevice->CreateCommittedResource(
                &bufferHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)));
        }

        UINT8* dstPtr = nullptr;
        _ThrowIfFailed(upload->Map(0, nullptr, (void**)&dstPtr));
        memcpy(dstPtr, data, size);
        upload->Unmap(0, nullptr);

        if (dxTarget->mState != D3D12_RESOURCE_STATE_COPY_DEST)
        {
            D3D12_RESOURCE_BARRIER toCopy = {};
            toCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            toCopy.Transition.pResource = dxTarget->mResource.Get();
            toCopy.Transition.Subresource = 0;
            toCopy.Transition.StateBefore = dxTarget->mState;
            toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            mCommandList->ResourceBarrier(1, &toCopy);
            dxTarget->mState = D3D12_RESOURCE_STATE_COPY_DEST;
        }

        mCommandList->CopyBufferRegion(dxTarget->mResource.Get(), 0, upload.Get(), 0, size);

        D3D12_RESOURCE_BARRIER onFinish = {};
        onFinish.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        onFinish.Transition.pResource = dxTarget->mResource.Get();
        onFinish.Transition.Subresource = 0;
        onFinish.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        onFinish.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        mCommandList->ResourceBarrier(1, &onFinish);
        dxTarget->mState = D3D12_RESOURCE_STATE_GENERIC_READ;

        mUploadResources.push_back(upload);
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
            dxBarrier.Transition.StateBefore = DX12Utils::transferResourceState(barrier.before);
            dxBarrier.Transition.StateAfter = DX12Utils::transferResourceState(barrier.after);
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
        ComPtr<ID3D12Resource> upload;
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
            _ThrowIfFailed(dxDevice->CreateCommittedResource(&bufferHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)));
        }
        
        // 3. 将数据从 std::vector<UINT8> 复制到上传堆中
        const UINT8* srcPtr = source.data();
        UINT8* dstPtr;
        _ThrowIfFailed(upload->Map(0, nullptr, (void**) &dstPtr));
        for (UINT row = 0; row < numRows; ++row)
        {
            memcpy(dstPtr, srcPtr, rowSizeInBytes);
            dstPtr += uploadFootprint.Footprint.RowPitch;
            srcPtr += rowSizeInBytes;
        }
        upload->Unmap(0, nullptr);
        
        if (dxTarget->mState != D3D12_RESOURCE_STATE_COPY_DEST)
        {
            D3D12_RESOURCE_BARRIER toCopy = {};
            toCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            toCopy.Transition.pResource = dxTarget->mResource.Get();
            toCopy.Transition.Subresource = 0;
            toCopy.Transition.StateBefore = dxTarget->mState;
            toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            mCommandList->ResourceBarrier(1, &toCopy);
            dxTarget->mState = D3D12_RESOURCE_STATE_COPY_DEST;
        }

        // 4. 将数据从上传堆复制到纹理资源中
        {
            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = upload.Get();
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
            dxTarget->mState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }

        mUploadResources.push_back(upload);
    }
    
    DX12Device::DX12Device()
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
        
        for (UINT i = 0; i < kFrameCount; i++)
        {
            _ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocators[i])));
        }
        
        memset(mFenceValues, 0, sizeof(UINT64) * kFrameCount);
        _ThrowIfFailed(mDevice->CreateFence(mFenceValues[mFrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
        mFenceValues[mFrameIndex]++;
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
    
    DX12Surface::DX12Surface(DX12Device& device, void* windowHandle, uint32_t windowWidth, uint32_t windowHeight)
        : mDevice(device)
        , mWindowHandle(windowHandle)
        , mWidth(windowWidth)
        , mHeight(windowHeight)
    {
        _ThrowIfFailed(mDevice.mDXGIFactory->MakeWindowAssociation((HWND) windowHandle, DXGI_MWA_NO_ALT_ENTER));

        ComPtr<IDXGISwapChain> swapchain;
        DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
        swapchainDesc.BufferCount = kFrameCount;
        swapchainDesc.BufferDesc.Width = windowWidth;
        swapchainDesc.BufferDesc.Height = windowHeight;
        swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.OutputWindow = (HWND) windowHandle;
        swapchainDesc.Windowed = TRUE;
        _ThrowIfFailed(mDevice.mDXGIFactory->CreateSwapChain(mDevice.mCommandQueue.Get(), &swapchainDesc, &swapchain));
        _ThrowIfFailed(swapchain.As(&mSwapChain));

        createTargets();
        mDevice.mSurfaces.push_back(this);
    }

    DX12Surface::~DX12Surface()
    {
        mDevice.waitForGPU();
        for (uint32_t i = 0; i < kFrameCount; i++)
        {
            mRenderTargets[i].reset();
            mDepthTargets[i].reset();
        }
        mRTVHeap.reset();
        mDSVHeap.reset();
        mSwapChain.Reset();
        detach();
    }

    void DX12Surface::createTargets()
    {
        u32_t numHandlePerHeap = max(kFrameCount, 8);
        mRTVHeap = std::make_shared<DX12DescriptorHeap>(mDevice, DX12DescriptorHeap::Usage::RTV, numHandlePerHeap);

        for (UINT i = 0; i < kFrameCount; i++)
        {
            mRenderTargets[i] = std::make_shared<DX12RenderTarget>();
            DX12RenderTarget* dxRT = dynamic_cast<DX12RenderTarget*>(mRenderTargets[i].get());
            _ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&dxRT->mResource)));
            dxRT->mView = mRTVHeap->acquire();
            mDevice.mDevice->CreateRenderTargetView(dxRT->mResource.Get(), nullptr, dxRT->mView);
        }

        mDSVHeap = std::make_shared<DX12DescriptorHeap>(mDevice, DX12DescriptorHeap::Usage::DSV, kFrameCount);
        for (UINT i = 0; i < kFrameCount; i++)
        {
            D3D12_RESOURCE_DESC depthDesc = {};
            depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            depthDesc.Width = mWidth;
            depthDesc.Height = mHeight;
            depthDesc.DepthOrArraySize = 1;
            depthDesc.MipLevels = 1;
            depthDesc.Format = DX12Utils::transferFormat(Format::D32_FLOAT);
            depthDesc.SampleDesc.Count = 1;
            depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format = DX12Utils::transferFormat(Format::D32_FLOAT);
            clearValue.DepthStencil.Depth = 1.0f;
            clearValue.DepthStencil.Stencil = 0;

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

            mDepthTargets[i] = std::make_shared<DX12RenderTarget>();
            DX12RenderTarget* dxDS = dynamic_cast<DX12RenderTarget*>(mDepthTargets[i].get());
            _ThrowIfFailed(mDevice.mDevice->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &depthDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearValue,
                IID_PPV_ARGS(&dxDS->mResource)));
            dxDS->mView = mDSVHeap->acquire();
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DX12Utils::transferFormat(Format::D32_FLOAT);
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            mDevice.mDevice->CreateDepthStencilView(dxDS->mResource.Get(), &dsvDesc, dxDS->mView);
        }

        mFrameBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    }

    void DX12Surface::detach()
    {
        for (auto it = mDevice.mSurfaces.begin(); it != mDevice.mSurfaces.end(); ++it)
        {
            if (*it == this)
            {
                mDevice.mSurfaces.erase(it);
                break;
            }
        }
    }

    void* DX12Surface::getWindowHandle() const
    {
        return mWindowHandle;
    }

    uint32_t DX12Surface::getWidth() const
    {
        return mWidth;
    }

    uint32_t DX12Surface::getHeight() const
    {
        return mHeight;
    }

    RenderTarget::Ref DX12Surface::getActiveRenderTarget()
    {
        return mRenderTargets[mFrameBufferIndex];
    }

    RenderTarget::Ref DX12Surface::getActiveDepthTarget()
    {
        return mDepthTargets[mFrameBufferIndex];
    }

    void DX12Surface::present()
    {
        HRESULT hr = mSwapChain->Present(1, 0);
        if (FAILED(hr))
        {
            _ThrowIfFailed(mDevice.mDevice->GetDeviceRemovedReason());
        }
        mFrameBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    }

    void DX12Surface::resize(uint32_t width, uint32_t height)
    {
        mDevice.waitForGPU();
        for (uint32_t i = 0; i < kFrameCount; i++)
        {
            mRenderTargets[i].reset();
            mDepthTargets[i].reset();
        }
        mRTVHeap.reset();
        mDSVHeap.reset();

        _ThrowIfFailed(mSwapChain->ResizeBuffers(kFrameCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
        mWidth = width;
        mHeight = height;
        createTargets();
    }

    uint32_t DX12Device::getFrameIndex() const
    {
        return mFrameIndex;
    }

    StaticBuffer::Ref DX12Device::createStaticBuffer(uint32_t length)
    {
        return std::make_shared<DX12StaticBuffer>(*this, length);
    }

    MutableBuffer::Ref DX12Device::createMutableBuffer(uint32_t length)
    {
        return std::make_shared<DX12MutableBuffer>(*this, length);
    }

    DynamicBuffer::Ref DX12Device::createDynamicBuffer(uint32_t length)
    {
        return std::make_shared<DX12DynamicBuffer>(*this, length);
    }
    
    Texture::Ref DX12Device::createTexture(const TextureOptions& options)
    {
        return std::make_shared<DX12Texture>(*this, options);
    }
    
    Program::Ref DX12Device::createProgram(const ProgramOptions& options)
    {
        return std::make_shared<DX12Program>(*this, options);
    }
    
    PipelineObject::Ref DX12Device::createPipelineObject()
    {
        return std::make_shared<DX12PipelineObject>(*this);
    }

    PipelineBindings::Ref DX12Device::createPipelineBindings(const PipelineLayout& layout)
    {
        return std::make_shared<DX12PipelineBindings>(*this, layout);
    }

    PipelineBindings::Ref DX12Device::createPipelineBindings(PipelineObject::Ref pipeline)
    {
        if (!pipeline)
        {
            throw std::runtime_error("Device: createPipelineBindings requires a pipeline.");
        }
        return createPipelineBindings(pipeline->getLayout());
    }

    CommandBuffer::Ref DX12Device::createCommandBuffer()
    {
        return std::make_shared<DX12CommandBuffer>(*this);
    }
    
    void DX12Device::commitCommandBuffer(const CommandBuffer::Ref commandBuffer)
    {
        const DX12CommandBuffer* dxCommandBuffer = dynamic_cast<const DX12CommandBuffer*>(commandBuffer.get());
        ID3D12CommandList* commandLists[] = {dxCommandBuffer->mCommandList.Get()};
        mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
    }
    
    void DX12Device::waitForGPU()
    {
        const UINT64 currentFenceValue = mFenceValues[mFrameIndex];
        
        _ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), currentFenceValue));
        
        _ThrowIfFailed(mFence->SetEventOnCompletion(currentFenceValue, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
        
        mFenceValues[mFrameIndex] = currentFenceValue + 1;
        _ThrowIfFailed(mCommandAllocators[mFrameIndex]->Reset());
    }
    
    void DX12Device::waitForNextFrame()
    {
        const UINT64 currentFenceValue = mFenceValues[mFrameIndex];
        _ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), currentFenceValue));

        mFrameIndex = (mFrameIndex + 1) % kFrameCount;

        const UINT64 completed = mFence->GetCompletedValue();
        if (completed < mFenceValues[mFrameIndex])
        {
            _ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValues[mFrameIndex], mFenceEvent));
            WaitForSingleObject(mFenceEvent, INFINITE);
        }

        mFenceValues[mFrameIndex] = currentFenceValue + 1;
        _ThrowIfFailed(mCommandAllocators[mFrameIndex]->Reset());
    }

    Surface::Ref DX12Device::createSurface(void* windowHandle, uint32_t windowWidth, uint32_t windowHeight)
    {
        for (DX12Surface* existing : mSurfaces)
        {
            if (existing->mWindowHandle == windowHandle)
            {
                throw std::runtime_error("Device: surface already exists for this window.");
            }
        }
        return std::make_shared<DX12Surface>(*this, windowHandle, windowWidth, windowHeight);
    }
    
    Device::Ref GPUFactory::createDevice()
    {
        return std::make_shared<DX12Device>();
    }
}
