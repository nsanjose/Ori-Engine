/*	=====================================================================================================
		Pixel Shader
		Tone Map (Plus)

	-	The primary purpose of this shader is to apply a tone mapping operator, although it also
		applies applies other features that are order sensitive.

	1.	Eye Adaptive Exposure
	2.	Tone Mapping
	3.	Bloom
	=====================================================================================================	*/
#include "common_hdr.hlsl"

SamplerState sampler_point								: register(s0);
SamplerState sampler_bilinear							: register(s1);
Texture2D source_texture								: register(t0);
Texture2D<float1> adapted_luminance_texture_max_mip		: register(t1);
Texture2D bloom_texture									: register(t2);

struct QuadVsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
};

float4 main(QuadVsOut input) : SV_TARGET
{
	float3 source_texture_color		= source_texture.Sample(sampler_point, input.tex_coord).rgb;
	float average_adapted_luminance	= AverageLuminanceFromMaxMip(sampler_point, adapted_luminance_texture_max_mip);
	float3 exposed_color			= Expose(source_texture_color, average_adapted_luminance);
	//float3 tone_mapped_color		= ToneMap_Reinhard_Detail(exposed_color);
	float3 tone_mapped_color		= ToneMap_Reinhard_BurnControl(exposed_color, 0.3f);
	//float3 tone_mapped_color		= ToneMap_Filmic_Uncharted2(exposed_color);
	float3 bloom					= bloom_texture.Sample(sampler_bilinear, input.tex_coord).rgb;
	return float4(tone_mapped_color + bloom, 1);
}