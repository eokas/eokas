#pragma pack_matrix(row_major)

cbuffer Transform : register(b0)
{
    float4x4 world;
    float4x4 wvp;
    float4 cameraPos;
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
    float4x4 shadowMatrix;
    float4 shadowParams;
};

cbuffer MaterialParams : register(b2)
{
    float4 albedo;
    float metallic;
    float roughness;
    float ao;
    float receiveShadow;
    float4 emissive;
};

Texture2D gAlbedoTexture : register(t0);
SamplerState gAlbedoSampler : register(s0);
Texture2D gShadowMap : register(t1);
SamplerState gShadowSampler : register(s1);

static const float kPI = 3.14159265;

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

float D_GGX(float NdotH, float a)
{
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(kPI * d * d, 1e-7);
}

float G_SchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / max(NdotX * (1.0 - k) + k, 1e-7);
}

float3 F_Schlick(float cosTheta, float3 F0)
{
    float f = pow(1.0 - saturate(cosTheta), 5.0);
    return F0 + (1.0 - F0) * f;
}

void evalLightRadiance(LightData l, float3 worldPos, out float3 L, out float3 radiance)
{
    L = float3(0, 0, 1);
    radiance = float3(0, 0, 0);

    int type = (int)l.typeInner.x;
    float3 col = l.colorIntensity.rgb * l.colorIntensity.a;
    if (type == 0)
    {
        L = normalize(-l.dirSpot.xyz);
        radiance = col;
    }
    else
    {
        float3 toLight = l.posRange.xyz - worldPos;
        float dist = length(toLight);
        L = toLight / max(dist, 1e-5);
        float atten = saturate(1.0 - dist / max(l.posRange.w, 1e-5));
        if (type == 2)
        {
            float cosOuter = l.dirSpot.w;
            float cosInner = l.typeInner.y;
            float cosA = dot(normalize(l.dirSpot.xyz), -L);
            float spot = saturate((cosA - cosOuter) / max(cosInner - cosOuter, 1e-5));
            atten *= spot;
        }
        radiance = col * atten;
    }
}

float evaluateShadow(float3 worldPos, float3 N, float3 L)
{
    float3 pos = worldPos + N * 0.08;
    float4 clip = mul(float4(pos, 1.0), shadowMatrix);
    float3 ndc = clip.xyz / max(clip.w, 1e-5);
    float2 uv = float2(ndc.x * 0.5 + 0.5, ndc.y * -0.5 + 0.5);
    float bias = shadowParams.x * (1.0 + 2.5 * (1.0 - saturate(dot(N, L))));
    float texel = shadowParams.y;
    float lit = 0.0;
    float inside = (uv.x > 0.0 && uv.x < 1.0 && uv.y > 0.0 && uv.y < 1.0 && ndc.z > 0.0 && ndc.z < 1.0) ? 1.0 : 0.0;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 suv = uv + float2(x, y) * texel;
            float closest = gShadowMap.SampleLevel(gShadowSampler, suv, 0).r;
            float kept = (ndc.z - bias <= closest) ? 1.0 : 0.0;
            lit += kept;
        }
    }
    float visibility = lit / 9.0;
    return lerp(1.0, visibility, inside);
}

float3 evalCookTorrance(float3 N, float3 V, float3 L, float3 radiance, float3 baseColor, float metal, float rough)
{
    float3 H = normalize(V + L);
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));
    float a = max(rough * rough, 0.0016);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), baseColor, metal);
    float3 F = F_Schlick(HdotV, F0);
    float D = D_GGX(NdotH, a);
    float G = G_SchlickGGX(NdotV, rough) * G_SchlickGGX(NdotL, rough);
    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-5);
    float3 kD = (1.0 - F) * (1.0 - metal);
    float3 diffuse = kD * baseColor / kPI;
    return (diffuse + specular) * radiance * NdotL;
}

float4 PSMain(Varying input) : SV_TARGET
{
    float4 texColor = gAlbedoTexture.Sample(gAlbedoSampler, input.uv);
    float4 base = texColor * input.color * albedo;
    float3 N = normalize(input.normal);
    float3 V = normalize(cameraPos.xyz - input.worldPos);
    float rough = max(roughness, 0.04);

    float3 lit = ambientAndCount.xyz * base.rgb * ao * (1.0 - metallic);
    int count = (int)ambientAndCount.w;
    for (int i = 0; i < 4; ++i)
    {
        if (i >= count)
            break;
        float3 L;
        float3 radiance;
        evalLightRadiance(lights[i], input.worldPos, L, radiance);
        float visibility = 1.0;
        int sunIndex = (int)(shadowParams.w + 0.5);
        if (receiveShadow > 0.5 && shadowParams.z > 0.5 && i == sunIndex)
            visibility = evaluateShadow(input.worldPos, N, L);
        lit += evalCookTorrance(N, V, L, radiance, base.rgb, metallic, rough) * visibility;
    }
    lit += emissive.rgb;
    return float4(lit, base.a);
}
