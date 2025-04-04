#ifndef _EOKAS_GPU_HEADER_H_
#define _EOKAS_GPU_HEADER_H_

#include "base/main.h"

namespace eokas
{
    enum class Format
    {
        Unknown,
        R32_UINT,
        R32_FLOAT,
        R8G8B8A8_UNORM,
        B8G8R8A8_UNORM,
        R16G16B16A16_FLOAT,
        R32G32_FLOAT,
        R32G32B32_FLOAT,
        R32G32B32A32_FLOAT,
        D32_FLOAT,
        D24_UNORM_S8_UINT,
        D16_UNORM,
        DXT1,
        DXT3,
        DXT5,
        ASTC_4x4_HDR,
        ASTC_6x6_HDR,
        ASTC_8x8_HDR,
        ASTC_10x10_HDR,
        ASTC_12x12_HDR
    };
    
    enum class Topology
    {
        Undefined = 0,
        PointList = 1,
        LineList = 2,
        LineStrip = 3,
        TriangleList = 4,
        TriangleStrip = 5,
        LineList_Adj = 10,
        LineStrip_Adj = 11,
        TriangleList_Adj = 12,
        TriangleStrip_Adj = 13,
    };
    
    struct Resource
    {
        using Ref = std::shared_ptr<Resource>;
        
        virtual void* getNativeResource() const = 0;
    };
    
    struct RenderTarget : public Resource
    {
        using Ref = std::shared_ptr<RenderTarget>;
    };
    
    struct DynamicBuffer : public Resource
    {
        using Ref = std::shared_ptr<DynamicBuffer>;
        virtual void* map() = 0;
        virtual void unmap() = 0;
    };
    
    struct TextureOptions
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 0;
        Format format = Format::Unknown;
        uint32_t mipCount = 1;
    };
    
    struct Texture : public Resource
    {
        using Ref = std::shared_ptr<Texture>;
        
        virtual const TextureOptions& getOptions() const = 0;
    };
    
    struct VertexElement
    {
        std::string semanticName = {};
        uint32_t semanticIndex = 0;
        uint32_t offset = 0;
        Format format = Format::Unknown;
    };
    
    enum class ProgramType
    {
        Vertex, Fragment, Compute
    };
    
    enum class ProgramTarget
    {
        SM_3_0, SM_3_1, SM_4_5, SM_5_0, SM_6_0, SM_6_8
    };
    
    struct ProgramOptions
    {
        std::string name = "";
        std::string source = "";
        std::string entry = "";
        ProgramType type = ProgramType::Vertex;
        ProgramTarget target = ProgramTarget::SM_3_0;
    };
    
    struct Program
    {
        using Ref = std::shared_ptr<Program>;
        
        virtual const ProgramOptions& getOptions() const = 0;
        virtual uint32_t getTextureCount() const = 0;
    };
    
    enum class FillMode
    {
        Solid, Wireframe
    };
    
    enum class CullMode
    {
        None, Front, Back
    };
    
    struct PipelineState
    {
        using Ref = std::shared_ptr<PipelineState>;
        
        virtual void begin() = 0;
        virtual void setVertexElements(std::vector<VertexElement>& vElements) = 0;
        virtual void setProgram(ProgramType type, Program::Ref program) = 0;
        virtual void setTexture(uint32_t index, Texture::Ref texture) = 0;
        virtual void setFillMode(FillMode fillMode) = 0;
        virtual void setCullMode(CullMode cullMode) = 0;
        virtual void end() = 0;
    };
    
    struct Viewport
    {
        float left = 0;
        float right = 1024;
        float top = 0;
        float bottom = 768;
        float front = 0.01f;
        float back = 1000.0f;
    };
    
    struct Barrier
    {
        Resource::Ref resource = nullptr;
        uint32_t type = 0;
        uint32_t before = 0;
        uint32_t after = 0;
    };
    
    struct CommandBuffer
    {
        using Ref = std::shared_ptr<CommandBuffer>;
        
        virtual void reset(PipelineState::Ref pso) = 0;
        virtual void setRenderTargets(const std::vector<RenderTarget::Ref>& renderTargets) = 0;
        virtual void clearRenderTarget(RenderTarget::Ref renderTarget, float(& color)[4]) = 0;
        virtual void setViewport(const Viewport& viewport) = 0;
        virtual void setTopology(Topology topology) = 0;
        virtual void setVertexBuffer(DynamicBuffer::Ref buffer, uint32_t length, uint32_t stride) = 0;
        virtual void setIndexBuffer(DynamicBuffer::Ref buffer, uint32_t length, Format format) = 0;
        virtual void drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) = 0;
        virtual void fillTexture(Texture::Ref target, const std::vector<uint8_t>& source) = 0;
        virtual void barrier(const std::vector<Barrier>& barriers) = 0;
        virtual void finish() = 0;
    };
    
    struct Device
    {
        using Ref = std::shared_ptr<Device>;
        
        virtual RenderTarget::Ref getActiveRenderTarget() = 0;
        virtual DynamicBuffer::Ref createDynamicBuffer(uint32_t length, uint32_t usage) = 0;
        virtual Texture::Ref createTexture(const TextureOptions& options) = 0;
        virtual Program::Ref createProgram(const ProgramOptions& options) = 0;
        virtual PipelineState::Ref createPipelineState() = 0;
        virtual CommandBuffer::Ref createCommandBuffer(PipelineState::Ref pso) = 0;
        virtual void commitCommandBuffer(CommandBuffer::Ref commandBuffer) = 0;
        virtual void present() = 0;
        virtual void waitForGPU() = 0;
        virtual void waitForNextFrame() = 0;
    };
    
    struct GPUFactory
    {
        static Device::Ref createDevice(void* windowHandle, uint32_t windowWidth, uint32_t windowHeight);
    };
}

#endif//_EOKAS_GPU_HEADER_H_
