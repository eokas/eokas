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
