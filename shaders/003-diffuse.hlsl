#pragma pack_matrix(row_major)

cbuffer Transform : register(b0)
{
    float4x4 world;
    float4x4 wvp;
};

Texture2D gMainTexture : register(t0);
SamplerState gMainSampler : register(s0);

struct Vertex
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct Varying
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

Varying VSMain(Vertex input)
{
    Varying ret;
    ret.position = mul(float4(input.position.xyz, 1.0), wvp);
    ret.normal = mul(float4(input.normal.xyz, 0.0), world).xyz;
    ret.color = input.color;
    ret.uv = input.uv;
    return ret;
}

float4 PSMain(Varying input) : SV_TARGET
{
    float4 baseColor = gMainTexture.Sample(gMainSampler, input.uv) * input.color;
    float3 n = normalize(input.normal);
    float3 lightDir = float3(0.0, 1.0, 0.0); // 从 +Y 打下，指向光源
    float diffuse = saturate(dot(n, lightDir));
    float lighting = 0.25 + 0.75 * diffuse;
    return baseColor * lighting;
}
