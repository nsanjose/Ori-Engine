/*	=====================================================================================================
		Pixel Shader
		Deferred Shading
		Accumulate Lighting - IBL
	=====================================================================================================	*/
#include "common_deferred.hlsl"
#include "common_pbr.hlsl"

cbuffer ConstantBuffer : register(b0)
{
	// reconstructing position_ws
	float4x4 inverse_view_matrix		: packoffset(c0);
	float4x4 inverse_projection_matrix	: packoffset(c4);
	float3 camera_position_world_space	: packoffset(c8);
	float camera_projection_a			: packoffset(c8.w);
	float camera_projection_b			: packoffset(c9);
};

SamplerState sampler_point					: register(s0);
Texture2D gbuffer_0							: register(t0);
Texture2D gbuffer_1							: register(t1);
Texture2D gbuffer_2							: register(t2);
Texture2D gbuffer_3							: register(t3);

SamplerState sampler_ansio					: register(s1);
TextureCube irradiance_map					: register(t4);
TextureCube prefiltered_env_map				: register(t5);
Texture2D<float2> brdf_lut					: register(t6);

struct VsOut
{
	float4 position					: SV_POSITION;
	float2 tex_coord				: TEXCOORD;
};

float4 main(VsOut input) : SV_TARGET
{
/*	=====================================================================================================
		Accessing G-Buffer
	=====================================================================================================
	Layout :	Rgba16f		|				  base color					|	 shadows	|
				Rgba16f		|		 encoded normal			|   metalness	|   roughness	|
				Rgba16f		|		x		|		x		|		x		|	   ao		|
				D24S8		|				  hardware depth				|    stencil	|
	-----------------------------------------------------------------------------------------------------	*/
	float3 base_color		= gbuffer_0.Sample(sampler_point, input.tex_coord).rgb;
	float2 encoded_normal	= gbuffer_1.Sample(sampler_point, input.tex_coord).rg;
	float metalness			= gbuffer_1.Sample(sampler_point, input.tex_coord).b;
	float roughness			= gbuffer_1.Sample(sampler_point, input.tex_coord).a;
	float ao				= gbuffer_2.Sample(sampler_point, input.tex_coord).a;
	float depth				= gbuffer_3.Sample(sampler_point, input.tex_coord).r;
/*	=====================================================================================================
		Reconstructions from G-Buffer
	=====================================================================================================	*/
	float3 normal = DecodeNormal_StereographicProjection(encoded_normal);
	// -----------------------------------------------------------------------------------------------------
	float4 position_clip_space = float4(input.tex_coord * 2 - 1, depth, 1);
	position_clip_space.y *= -1;									
	float4 position_view_space = mul(position_clip_space, inverse_projection_matrix);
	position_view_space /= position_view_space.w;
	float3 position_world_space = mul(position_view_space, inverse_view_matrix).xyz;
/*	=====================================================================================================
		Lighting
	=====================================================================================================	*/	/*
		Pre - Calculations
	-----------------------------------------------------------------------------------------------------	*/
	float3 F0 = { 0.04f, 0.04f, 0.04f };	// specular reflectance at normal incidence (~0.02-0.08 for non-metals, baseColor for metals)
	F0 = lerp(F0, base_color, metalness);
	float3 v = normalize(camera_position_world_space - position_world_space);	// view direction
	float3 n = normalize(normal);												// normal direction
	float3 r = normalize(reflect(v, n));										// reflection of v across n
	float NDotV = saturate(max(dot(n, v), 0.0001));
	// -----------------------------------------------------------------------------------------------------
	float irradiance = 2.0f;	// if point/spot apply falloff to irradiance
	float3 F = 0.0f;			// fresnel to copy (out) from specular function
/*	-----------------------------------------------------------------------------------------------------
		BRDF Calculations
	-----------------------------------------------------------------------------------------------------	*/
	float3 indirect_diffuse = irradiance_map.SampleLevel(sampler_ansio, n, 0).xyz * base_color;
	float3 indirect_specular = ApproximateSpecularIBL(prefiltered_env_map, brdf_lut, sampler_ansio, F0, roughness, r, NDotV);
/*	-----------------------------------------------------------------------------------------------------
		Conservation of Energy and Shadowing
	-----------------------------------------------------------------------------------------------------	*/
	//float3 ks = F;	// fresnel already applied in specular components
	float3 kd = (1.0f - F) * (1.0f - metalness);
	float3 lighting = kd * indirect_diffuse + /*ks **/ indirect_specular;
	float3 final_color = lighting * ao;

	return float4(final_color, 1.0f);
}