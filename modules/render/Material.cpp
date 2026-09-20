#include "./Material.h"
#include <cstring>

namespace eokas
{
    static const char* kDefaultSpaceHLSL = R"HLSL(
#pragma pack_matrix(row_major)

cbuffer Transform : register(b0)
{
    float4x4 world;
    float4x4 wvp;
};

struct LightData
{
    float4 colorIntensity;
    float4 posRange;
    float4 dirSpot;
    float4 typeInner;
};

cbuffer Lighting : register(b1)
{
    float4 ambientAndCount;
    LightData lights[4];
};

cbuffer MaterialParams : register(b2)
{
    float4 albedo;
};

Texture2D gMainTexture : register(t0);
SamplerState gMainSampler : register(s0);

struct VertexIn
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct Varying
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD1;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

Varying VSMain(VertexIn input)
{
    Varying ret;
    float4 wp = mul(float4(input.position.xyz, 1.0), world);
    ret.worldPos = wp.xyz;
    ret.position = mul(float4(input.position.xyz, 1.0), wvp);
    ret.normal = mul(float4(input.normal.xyz, 0.0), world).xyz;
    ret.color = input.color;
    ret.uv = input.uv;
    return ret;
}

float3 evalLight(LightData l, float3 n, float3 worldPos)
{
    float3 result = float3(0, 0, 0);
    int type = (int)l.typeInner.x;
    float3 col = l.colorIntensity.rgb * l.colorIntensity.a;
    if (type == 0)
    {
        float3 toLight = normalize(-l.dirSpot.xyz);
        result = col * saturate(dot(n, toLight));
    }
    else
    {
        float3 toLight = l.posRange.xyz - worldPos;
        float dist = length(toLight);
        toLight = toLight / max(dist, 1e-5);
        float atten = saturate(1.0 - dist / max(l.posRange.w, 1e-5));
        if (type == 2)
        {
            float cosOuter = l.dirSpot.w;
            float cosInner = l.typeInner.y;
            float cosA = dot(normalize(l.dirSpot.xyz), -toLight);
            float spot = saturate((cosA - cosOuter) / max(cosInner - cosOuter, 1e-5));
            atten *= spot;
        }
        result = col * saturate(dot(n, toLight)) * atten;
    }
    return result;
}

float4 PSMain(Varying input) : SV_TARGET
{
    float4 baseColor = gMainTexture.Sample(gMainSampler, input.uv) * input.color * albedo;
    float3 n = normalize(input.normal);
    float3 lit = ambientAndCount.xyz;
    int count = (int)ambientAndCount.w;
    for (int i = 0; i < 4; ++i)
    {
        if (i >= count) break;
        lit += evalLight(lights[i], n, input.worldPos);
    }
    return float4(baseColor.rgb * lit, baseColor.a);
}
)HLSL";

    void Material::build(Device::Ref device, DynamicBuffer::Ref spaceLighting)
    {
        if (built)
            return;

        const char* src = shaderSource.empty() ? kDefaultSpaceHLSL : shaderSource.c_str();

        ProgramOptions vsOpt;
        vsOpt.name = "005-space-vs";
        vsOpt.source = src;
        vsOpt.entry = "VSMain";
        vsOpt.type = ProgramType::Vertex;
        vsOpt.target = ProgramTarget::SM_5_0;
        vs = device->createProgram(vsOpt);

        ProgramOptions psOpt;
        psOpt.name = "005-space-ps";
        psOpt.source = src;
        psOpt.entry = "PSMain";
        psOpt.type = ProgramType::Fragment;
        psOpt.target = ProgramTarget::SM_5_0;
        ps = device->createProgram(psOpt);

        std::vector<VertexElement> vElements;
        vElements.push_back({"POSITION", 0, 0, Format::R32G32B32_FLOAT});
        vElements.push_back({"NORMAL", 0, 12, Format::R32G32B32_FLOAT});
        vElements.push_back({"COLOR", 0, 24, Format::R32G32B32A32_FLOAT});
        vElements.push_back({"TEXCOORD", 0, 40, Format::R32G32_FLOAT});

        pipelineObject = device->createPipelineObject();
        pipelineObject->begin();
        pipelineObject->setProgram(ProgramType::Vertex, vs);
        pipelineObject->setProgram(ProgramType::Fragment, ps);
        pipelineObject->setVertexElements(vElements);
        pipelineObject->setCullMode(CullMode::Front);
        DepthStencilState depthStencil;
        depthStencil.depthTest = true;
        depthStencil.depthWrite = true;
        depthStencil.depthFunc = CompareOp::Less;
        pipelineObject->setDepthStencilState(depthStencil);
        SamplerState sampler;
        sampler.minFilter = SamplerFilterMode::Linear;
        sampler.magFilter = SamplerFilterMode::Linear;
        sampler.mipFilter = SamplerFilterMode::Linear;
        sampler.addressU = SamplerAddressMode::Clamp;
        sampler.addressV = SamplerAddressMode::Clamp;
        sampler.addressW = SamplerAddressMode::Clamp;
        pipelineObject->setSamplerState(0, sampler);
        BlendState blend;
        blend.enabled = true;
        blend.srcColor = BlendFactor::SrcAlpha;
        blend.dstColor = BlendFactor::OneMinusSrcAlpha;
        blend.colorOp = BlendOp::Add;
        blend.srcAlpha = BlendFactor::One;
        blend.dstAlpha = BlendFactor::OneMinusSrcAlpha;
        blend.alphaOp = BlendOp::Add;
        pipelineObject->setBlendState(blend);
        pipelineObject->end();

        materialUniforms = device->createDynamicBuffer(sizeof(MaterialUniforms), (uint32_t)BufferUsage::UniformBuffer);
        updateUniforms();

        if (!albedoTexture)
        {
            TextureOptions options;
            options.width = 1;
            options.height = 1;
            options.mipCount = 1;
            options.format = Format::R8G8B8A8_UNORM;
            defaultTexture = device->createTexture(options);
        }

        pipelineBindings = device->createPipelineBindings(pipelineObject);
        pipelineBindings->begin();
        pipelineBindings->setUniformBufferByName("Lighting", spaceLighting);
        pipelineBindings->setUniformBufferByName("MaterialParams", materialUniforms);
        pipelineBindings->setTextureByName("gMainTexture", albedoTexture ? albedoTexture : defaultTexture);
        pipelineBindings->end();

        built = true;
    }

    void Material::updateUniforms()
    {
        if (!materialUniforms)
            return;
        MaterialUniforms mu{};
        mu.albedo[0] = albedo.r;
        mu.albedo[1] = albedo.g;
        mu.albedo[2] = albedo.b;
        mu.albedo[3] = albedo.a;
        void* ptr = materialUniforms->map();
        memcpy(ptr, &mu, sizeof(mu));
        materialUniforms->unmap();
    }

    void Material::uploadDefaultTexture(CommandBuffer::Ref cmd)
    {
        if (textureUploaded || !cmd)
            return;
        if (defaultTexture)
        {
            std::vector<uint8_t> white = {255, 255, 255, 255};
            cmd->fillTexture(defaultTexture, white);
        }
        textureUploaded = true;
    }
}
