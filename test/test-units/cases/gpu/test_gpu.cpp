#include "Unit.h"
#include "gpu/main.h"

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
