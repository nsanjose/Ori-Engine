/*	=====================================================================================================
		Pixel Shader
		Screen Space Ambient Occlusion
	=====================================================================================================	*/
#include "common_deferred.hlsl"

cbuffer PerFrame : register(b0)
{
	float4x4 inverse_projection_matrix;
	float4x4 projection_matrix;
	float4x4 view_matrix;
};

cbuffer PerWindowChange : register(b1)
{
	// tiling noise map
	float2 screen_size		: packoffset(c0);
	float2 noise_map_size	: packoffset(c0.z);
}

SamplerState sampler_point	: register(s0);
Texture2D gbuffer_1			: register(t0);
Texture2D gbuffer_3			: register(t1);

// noise file from https://web.archive.org/web/20150326040127/http://www.gamerendering.com:80/category/lighting/ssao-lighting/
Texture2D noise_map : register(t2);

// improve sample kernel radii distribution to make better use of less samples http://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html
// choose seed for baking my own noise map
static const uint m_SAMPLE_COUNT = 14;
static const float3 kernel[m_SAMPLE_COUNT] = { float3(1, 0, 0), float3(-1, 0, 0), float3(0, 1, 0), float3(0, -1, 0), float3(0, 0, 1), float3(0, 0, -1), float3(.707, .707, .707), float3(-.707, .707, .707), float3(.707, -.707, .707), float3(-.707, -.707, .707), float3(.707, .707, -.707), float3(-.707, .707, -.707), float3(.707, -.707, -.707), float3(-.707, -.707, -.707) };

float calculate_occlusion(float4 position_vs, float3 sample_offset, float4x4 projection_matrix, float depth, float max_depth_difference, float3 normal)
{
	// Projecting sample to gbuffer texture coordinates
	float4 sample_position_vs = position_vs + float4(sample_offset, 0);
	float4 sample_position_cs = mul(sample_position_vs, projection_matrix);
	float2 sample_position_ndc = sample_position_cs.xy / sample_position_cs.w;
	sample_position_ndc.y *= -1;
	sample_position_ndc = sample_position_ndc * 0.5f + 0.5f;

	// Comparing sample and source
	float sample_depth = gbuffer_3.Sample(sampler_point, sample_position_ndc).r;
	float depth_difference = abs(depth - sample_depth);

	return
		step(sample_depth, depth)							// Skip anything behind the source
		* step(depth_difference, max_depth_difference)		// Skip anything too far apart to cause occlusion
		//* dot(sample_offset, normal);						// Scale contribution based on angle
		//* (1 - depth_difference / max_depth_difference);	// Scale contribution based on proximity
		* (1 - sqrt(length(sample_position_vs - position_vs)));
}

struct VsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
};

float4 main(VsOut input) : SV_TARGET
{
/*	=====================================================================================================
		Accessing G-Buffer
	=====================================================================================================
	Layout :	Rgba16f		|				  base color					|	 shadow		|
				Rgba16f		|		 encoded normal			|   metalness	|   roughness	|
				Rgba16f		|		x		|		x		|	  ssao		|	   ao		|
				D24S8		|				  hardware depth				|    stencil	|
	-----------------------------------------------------------------------------------------------------	*/
	float2 encoded_normal	= gbuffer_1.Sample(sampler_point, input.tex_coord).rg;
	float depth				= gbuffer_3.Sample(sampler_point, input.tex_coord).r;	
/*	=====================================================================================================
		Reconstructions from G-Buffer
	=====================================================================================================	*/
	float3 normal = DecodeNormal_StereographicProjection(encoded_normal);
	// -----------------------------------------------------------------------------------------------------
	float4 position_cs = float4(input.tex_coord * 2 - 1, depth, 1);
	position_cs.y *= -1;
	float4 position_vs = mul(position_cs, inverse_projection_matrix);
	position_vs /= position_vs.w;
/*	=====================================================================================================
		SSAO
	=====================================================================================================	*/	/*
		Settings
	-----------------------------------------------------------------------------------------------------	*/
	float sample_radius = .3f / (m_SAMPLE_COUNT / 2);
	float max_depth_difference = 1.f / position_vs.z; // scaled to avoid re-calculating linear depth
	float strength = 2;
	// --------------------------------------------------------------------------------------------------
	float occlusion = 0;
	for (uint sample_i = 0; sample_i < m_SAMPLE_COUNT; sample_i++)
	{
		// Locating a nearby position to sample within a...
		// Sphere - Crysis: flat surfaces are 50% self occluded, corners are 25% self occluded and brighter than flat surfaces
		float3 random = normalize(noise_map.Sample(sampler_point, input.tex_coord * screen_size / noise_map_size).xyz * 2 - 1);
		float3 sample_offset = reflect(kernel[sample_i], random);
		// (Normal Oriented) Hemisphere
		sample_offset = sign(dot(sample_offset, normal)) * sample_offset;

		occlusion += calculate_occlusion(position_vs, sample_offset * sample_radius * (sample_i % (m_SAMPLE_COUNT / 2) + 1), projection_matrix, depth, max_depth_difference, normal);
		//occlusion += calculate_occlusion(position_vs, sample_offset * sample_radius * 0.5f, projection_matrix, depth, max_depth_difference, normal);
		//occlusion += calculate_occlusion(position_vs, sample_offset * sample_radius, projection_matrix, depth, max_depth_difference, normal);
	}
	occlusion /= m_SAMPLE_COUNT;
	occlusion *= strength;

	return float4(0, 0, occlusion, 0);
}