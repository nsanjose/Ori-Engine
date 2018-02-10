/*	=====================================================================================================
		Pixel Shader
		Deferred Shading
		Second Pass: Accumulate shading from each light, its shadow, and gbuffer data
	=====================================================================================================	*/
#include "common_deferred.hlsl"
#include "common_lights.hlsl"
#include "common_pbr.hlsl"

AbstractLight light;

cbuffer LightDataPerPass : register(b0)
{
	Linked_DirectionalLight directional_light;
	Linked_SpotLight		spot_light;
	Linked_PointLight		point_light;
}

cbuffer CameraDataPerPass : register(b1)
{
	// reconstructing position_ws
	float4x4 inverse_view_matrix		: packoffset(c0);
	float4x4 inverse_projection_matrix	: packoffset(c4);
	float3 camera_position_world_space	: packoffset(c8);
};

SamplerState sampler_point	: register(s0);
Texture2D gbuffer_0			: register(t0);
Texture2D gbuffer_1			: register(t1);
Texture2D gbuffer_2			: register(t2);
Texture2D gbuffer_3			: register(t3);

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
	float3 base_color		= gbuffer_0.Sample(sampler_point, input.tex_coord).rgb;
	float shadow			= gbuffer_0.Sample(sampler_point, input.tex_coord).a;
	float2 encoded_normal	= gbuffer_1.Sample(sampler_point, input.tex_coord).rg;
	float metalness			= gbuffer_1.Sample(sampler_point, input.tex_coord).b;
	float roughness			= gbuffer_1.Sample(sampler_point, input.tex_coord).a;
	float ssao				= gbuffer_2.Sample(sampler_point, input.tex_coord).b;
	float ao				= gbuffer_2.Sample(sampler_point, input.tex_coord).a;
	float depth				= gbuffer_3.Sample(sampler_point, input.tex_coord).r;	
/*	=====================================================================================================
		Reconstructions from G-Buffer
	=====================================================================================================	*/
	float3 normal = DecodeNormal_StereographicProjection(encoded_normal);
	normal = mul(normal, inverse_view_matrix);
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
	float3 l = normalize(light.GetDirectionToLight(position_world_space));
	float3 v = normalize(camera_position_world_space - position_world_space);	// view direction
	float3 h = normalize(l + v);												// halfway (between l and v)
	float3 n = normalize(normal);												// normal direction
	float3 r = normalize(reflect(v, n));										// reflection of v across n
	float VDotH = saturate(max(dot(v, h), 0));
	float NDotL = saturate(max(dot(n, l), 0.0001));
	float NDotV = saturate(max(dot(n, v), 0.0001));
	float NDotH = saturate(max(dot(n, h), 0.0001));
/*	-----------------------------------------------------------------------------------------------------
		BRDF Calculations
	-----------------------------------------------------------------------------------------------------	*/
	float3 F = 0.0f; // fresnel to copy (out) from specular function
	float3 direct_diffuse = DirectDiffuseBRDF_Lambert(base_color, light.GetIrradiance(position_world_space), NDotL);
	float3 direct_specular = DirectSpecularBRDF_CookTorrence(roughness, F0, VDotH, NDotL, NDotV, NDotH, F);
/*	-----------------------------------------------------------------------------------------------------
		Conservation of Energy and Shadowing
	-----------------------------------------------------------------------------------------------------	*/
	//float3 ks = F;	// fresnel already applied in specular components
	float3 kd = (1.0f - F) * (1.0f - metalness);
	float3 lighting = kd * direct_diffuse + /*ks **/ direct_specular;
	float3 final_color = lighting * shadow * ao;	// should "ambient" occlusion apply only to indirect lighting?

	return float4(final_color, 1.0f);
}