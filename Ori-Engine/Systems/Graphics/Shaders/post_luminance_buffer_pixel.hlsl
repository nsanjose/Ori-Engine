/*	=====================================================================================================
		Pixel Shader
		Auto Exposure
		Luminance Buffering

	-	This shader stores a frame's luminance in a texture. It is the first step in calculating a 
		frame's average luminance for exposure calculation.
	-	In Eye Adaptive Exposure, the luminance is next temporally adapted before calculating the frame's
		average luminance.
	=====================================================================================================	*/
#include "common_hdr.hlsl"

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
	return LuminanceFromRgb(source_texture_color);
}