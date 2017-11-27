/*	=====================================================================================================
		Pixel Shader
		Blur
		1D Gaussian

	-	Influences a pixel's color by its neighbors' colors using a one dimensional Gaussian distribution.
	=====================================================================================================	*/

static const int SAMPLE_COUNT = 15;

cbuffer BufferParameters : register(b0)
{
	float4 sample_offsets_and_weights[SAMPLE_COUNT];	// Offsets: X,Y; Weights: Z; Emtpy: W;
}

SamplerState sampler_point		: register(s0);
Texture2D source_texture		: register(t0);

struct QuadVsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord		: TEXCOORD;
};

float4 main(QuadVsOut input) : SV_TARGET
{
	float2 sample_tex_coord;
	float3 sample_color;
	float3 final_color;

	for (int sample_i; sample_i < SAMPLE_COUNT; sample_i++)
	{
		sample_tex_coord = input.tex_coord + sample_offsets_and_weights[sample_i].xy;
		sample_color = source_texture.Sample(sampler_point, sample_tex_coord).rgb;
		final_color += sample_color * sample_offsets_and_weights[sample_i].z;
	}
	return float4(final_color, 1);
}