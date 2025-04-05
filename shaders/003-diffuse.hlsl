
Texture2D gMainTexture : register(t0);
SamplerState gMainSampler : register(s0);

struct Vertex
{
	float4 position: POSITION;
	float4 normal: NORMAL;
	float4 color: COLOR;
	float2 uv: TEXCOORD;
};

struct Varying
{
	float4 position: SV_POSITION;
	float4 normal: NORMAL;
	float4 color: COLOR;
	float2 uv: TEXCOORD;
};

Varying VSMain(Vertex input)
{
	Varying ret;

	ret.position = input.position;
	ret.normal = input.normal;
	ret.color = input.color;
	ret.uv = input.uv;

	return ret;
}

float4 PSMain(Varying input) : SV_TARGET
{
    float4 baseColor = gMainTexture.Sample(gMainSampler, input.uv) * input.color;

    float3 n = normalize(input.normal.xyz);
    float3 l = normalize(float3(0.4, 0.4, 0.5));
    float diffuse = max(dot(n, l), 0.0);

    float4 finalColor = baseColor * diffuse;

	return finalColor;
}
