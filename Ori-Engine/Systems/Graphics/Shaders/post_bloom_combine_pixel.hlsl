/*	=====================================================================================================
		Pixel Shader
		Bloom
		Combining

	-	The third step in the bloom effect, upsampling to combine blurred downsamples with sources.
	=====================================================================================================	*/

SamplerState sampler_bilinear	: register(s0);
Texture2D source_texture		: register(t0);

struct QuadVsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
};

float4 main(QuadVsOut input) : SV_TARGET
{
	return float4(source_texture.Sample(sampler_bilinear, input.tex_coord).rgb, 1);
}