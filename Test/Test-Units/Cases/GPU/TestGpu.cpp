#include "Unit.h"
#include "GPU/Main.h"

#include <stdexcept>

using namespace eokas;

EOKAS_TEST_CASE(pipeline) {
    PipelineLayout layout;
    layout.add({"Camera", PipelineResourceType::UniformBuffer, 0, 1});
    layout.add({"Camera", PipelineResourceType::UniformBuffer, 0, 1});
    EOKAS_EXPECT(layout.entries.size() == 1);
    EOKAS_EXPECT(layout.findByName(PipelineResourceType::UniformBuffer, "Camera") != nullptr);
    EOKAS_EXPECT(layout.findBySlot(PipelineResourceType::Texture, 0) == nullptr);

    PipelineLayout subset;
    subset.add({"Camera", PipelineResourceType::UniformBuffer, 0, 1});
    EOKAS_EXPECT(layout.compatibleWith(subset));
    subset.add({"Albedo", PipelineResourceType::Texture, 1, 1});
    EOKAS_EXPECT(!layout.compatibleWith(subset));

    SamplerState sampler;
    SamplerState same;
    EOKAS_EXPECT(sampler == same);
    same.addressU = SamplerAddressMode::Repeat;
    EOKAS_EXPECT(sampler != same);

    DepthStencilState depth;
    DepthStencilState depthCopy = depth;
    depthCopy.depthWrite = false;
    EOKAS_EXPECT(depth != depthCopy);

    BlendState blend;
    blend.enabled = true;
    BlendState blendCopy = blend;
    EOKAS_EXPECT(blend == blendCopy);

    bool conflict = false;
    try {
        layout.add({"Camera", PipelineResourceType::UniformBuffer, 2, 1});
    }
    catch (const std::runtime_error&) {
        conflict = true;
    }
    EOKAS_EXPECT(conflict);
    return 0;
}

EOKAS_TEST_CASE(device) {
    try {
        Device::Ref device = GPUFactory::createDevice();
        EOKAS_EXPECT(device != nullptr);
        EOKAS_EXPECT(device->getFrameIndex() < kFrameCount);
        StaticBuffer::Ref buffer = device->createStaticBuffer(64);
        EOKAS_EXPECT(buffer != nullptr);
        EOKAS_EXPECT(buffer->getLength() == 64);
    }
    catch (const std::exception& error) {
        printf("unit check failed: gpu device: %s\n", error.what());
        return 1;
    }
    return 0;
}

EOKAS_TEST_CASE(mrt_pso) {
    try {
        Device::Ref device = GPUFactory::createDevice();
        TextureOptions color0;
        color0.width = 4;
        color0.height = 4;
        color0.format = Format::R8G8B8A8_UNORM;
        color0.usage = TextureUsage::ColorAttachment;
        TextureOptions color1 = color0;
        TextureOptions depth;
        depth.width = 4;
        depth.height = 4;
        depth.format = Format::D32_FLOAT;
        depth.usage = TextureUsage::DepthStencilAttachment;

        Texture2D::Ref rtColor0 = device->createTexture2D(color0);
        Texture2D::Ref rtColor1 = device->createTexture2D(color1);
        Texture2D::Ref rtDepth = device->createTexture2D(depth);

        EOKAS_EXPECT(rtColor0->getOptions().width == 4);
        EOKAS_EXPECT(rtColor0->getOptions().height == 4);
        EOKAS_EXPECT(rtColor1->getOptions().format == Format::R8G8B8A8_UNORM);
        EOKAS_EXPECT(rtDepth->getOptions().format == Format::D32_FLOAT);
        EOKAS_EXPECT(rtColor0->getOptions().usage == TextureUsage::ColorAttachment);
        EOKAS_EXPECT(rtDepth->getOptions().usage == TextureUsage::DepthStencilAttachment);
        (void)rtColor1;

        static const char* vsSource = R"(
            struct VSOut { float4 pos : SV_POSITION; };
            VSOut VSMain(float3 pos : POSITION) {
                VSOut o;
                o.pos = float4(pos, 1.0);
                return o;
            }
        )";
        static const char* psSource = R"(
            struct PSOut {
                float4 c0 : SV_Target0;
                float4 c1 : SV_Target1;
            };
            PSOut PSMain() {
                PSOut o;
                o.c0 = float4(1, 0, 0, 1);
                o.c1 = float4(0, 1, 0, 1);
                return o;
            }
        )";

        ShaderOptions vsOptions;
        vsOptions.name = "mrt_vs";
        vsOptions.source = vsSource;
        vsOptions.entry = "VSMain";
        vsOptions.type = ShaderType::Vertex;
        vsOptions.target = ShaderTarget::SM_5_0;
        ShaderOptions psOptions;
        psOptions.name = "mrt_ps";
        psOptions.source = psSource;
        psOptions.entry = "PSMain";
        psOptions.type = ShaderType::Fragment;
        psOptions.target = ShaderTarget::SM_5_0;

        auto vs = device->createShaderProgram(vsOptions);
        auto ps = device->createShaderProgram(psOptions);

        std::vector<VertexElement> vElements;
        vElements.push_back({ "POSITION", 0, 0, Format::R32G32B32_FLOAT });

        PipelineObject::Ref pipeline = device->createPipelineObject();
        pipeline->begin();
        pipeline->setShaderProgram(ShaderType::Vertex, vs);
        pipeline->setShaderProgram(ShaderType::Fragment, ps);
        pipeline->setVertexElements(vElements);
        pipeline->setColorBufferFormats({ Format::R8G8B8A8_UNORM, Format::R8G8B8A8_UNORM });
        pipeline->setDepthStencilBufferFormat(Format::D32_FLOAT);
        pipeline->end();
    }
    catch (const std::exception& error) {
        printf("unit check failed: gpu mrt_pso: %s\n", error.what());
        return 1;
    }
    return 0;
}
