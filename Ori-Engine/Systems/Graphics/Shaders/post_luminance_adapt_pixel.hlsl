/*	=====================================================================================================
		Pixel Shader
		Eye Adaptive Exposure
		Adapt Luminance

	-	This shader emulates a delay in exposure balance based on the human eye's rod and cones reaction
		time. This is done by applying a curve to the luminance parameter of later exposure calculations.
		These adapted luminances are next averaged for use in exposure calculation.
	=====================================================================================================	*/	/*
		Temporal Adaptation [of Luminance]	http://www.diva-portal.se/smash/get/diva2:648041/FULLTEXT01.pdf
	-----------------------------------------------------------------------------------------------------	*/

cbuffer BufferParameters : register(b0)
{
	float delta_time;
	float adaption_rate;	// Rods: 0.4, Cones: 0.1, Example: .001 ~ 5sec
}

SamplerState sampler_point							: register(s0);
Texture2D<float1> current_luminance_texture			: register(t0);
Texture2D<float1> past_adapted_luminance_texture	: register(t1);

struct QuadVsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
};

float4 main(QuadVsOut input) : SV_TARGET
{
	float current_luminance = current_luminance_texture.Sample(sampler_point, input.tex_coord).r;
	float past_luminance = past_adapted_luminance_texture.Sample(sampler_point, input.tex_coord).r;
	return past_luminance + (current_luminance - past_luminance) * (1.0f - exp(-delta_time / adaption_rate));
}