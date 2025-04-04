

struct Vertex {
	float4 position: POSITION;
	float4 color: COLOR;
};

struct Varying {
	float4 position: SV_POSITION;
	float4 color: COLOR;
};

Varying VSMain(Vertex input) {
	Varying ret;

	ret.position = input.position;
	ret.color = input.color;

	return ret;
}

float4 PSMain(Varying input) : SV_TARGET{
	return input.color;
}
