/*	=====================================================================================================
		Pixel Shader
		Bloom
		Extraction

	-	The first step in the bloom effect, choosing what to blur.
	=====================================================================================================	*/
#include "common_hdr.hlsl"

static const float BRIGHTNESS_THRESHOLD = 0.5f;

SamplerState sampler_linear		: register(s0);
Texture2D source_texture		: register(t0);

struct QuadVsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
};

float4 main(QuadVsOut input) : SV_TARGET
{
	float3 source_texture_color = source_texture.Sample(sampler_linear, input.tex_coord).rgb;
	// Blooming hdr values produced white square artifacts from blur on extreme specular highlights.
	float3 tone_mapped_color = ToneMap_Filmic_Uncharted2(source_texture_color);		
	float3 final_color = .01f * tone_mapped_color;
	//float3 final_color = saturate((source_texture_color - BRIGHTNESS_THRESHOLD) / (1 - BRIGHTNESS_THRESHOLD));
	//float3 final_color = max(0, source_texture_color - BRIGHTNESS_THRESHOLD);
	return float4(final_color, 1);
}