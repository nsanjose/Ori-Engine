/*	=====================================================================================================
		Pixel Shader
		Bloom
		Combining

	-	The third step in the bloom effect, combining blurred downsamples with sources.
	-	When downsamples are used for blurring efficiency, each blurred downsample is combined with the next
		bigger downsample before that downsample then blurred until the first downsample is blurred.
	=====================================================================================================	*/

SamplerState sampler_linear		: register(s0);
Texture2D source_texture_one		: register(t0);
Texture2D source_texture_two		: register(t1);

struct QuadVsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
};

float4 main(QuadVsOut input) : SV_TARGET
{
	float3 source_texture_one_color = source_texture_one.Sample(sampler_linear, input.tex_coord).rgb;
	float3 source_texture_two_color = source_texture_two.Sample(sampler_linear, input.tex_coord).rgb;
	return float4(source_texture_one_color + source_texture_two_color, 1);
}