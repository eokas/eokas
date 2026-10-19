#pragma pack_matrix(row_major)

cbuffer Transform : register(b0)
{
    float4x4 world;
    float4x4 wvp;
    float4 cameraPos;
};

Texture2D gMainTexture : register(t0);
SamplerState gMainSampler : register(s0);

struct Vertex
{
    float2 position : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct Varying
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

Varying VSMain(Vertex input)
{
    Varying ret;
    ret.position = mul(float4(input.position, 0.0, 1.0), wvp);
    ret.uv = input.uv;
    ret.color = input.color;
    return ret;
}

float4 PSMain(Varying input) : SV_TARGET
{
    return gMainTexture.Sample(gMainSampler, input.uv) * input.color;
}
