

Texture2D gMainTexture : register(t0);
SamplerState gMainSampler : register(s0);

struct Vertex {
	float4 position: POSITION;
	float4 color: COLOR;
	float2 uv: TEXCOORD;
};

struct Varying {
	float4 position: SV_POSITION;
	float4 color: COLOR;
	float2 uv: TEXCOORD;
};

Varying VSMain(Vertex input) {
	Varying ret;

	ret.position = input.position;
	ret.color = input.color;
	ret.uv = input.uv;

	return ret;
}

float4 PSMain(Varying input) : SV_TARGET{
	return gMainTexture.Sample(gMainSampler, input.uv) * input.color;
}
