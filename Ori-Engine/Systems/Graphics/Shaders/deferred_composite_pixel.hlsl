/*	=====================================================================================================
		Pixel Shader
		Deferred Shading
		Second Pass: Composite lighting with GBuffers
	=====================================================================================================	*/
#include "common_deferred.hlsl"
#include "common_lights.hlsl"
#include "common_pbr.hlsl"

cbuffer BufferParameters : register(b0)
{
	// Lighting
	DirectionalLight dL1;					//: packoffset(c0);
	// Reconstructing positionWS
	float4x4 inverse_view_matrix;			//: packoffset(c3);
	float4x4 inverse_projection_matrix;		//: packoffset(c7);
	float3 camera_position_world_space;		//: packoffset(c8);
	float camera_projection_a;				//: packoffset(c2.w);
	float camera_projection_b;				//: packoffset(c8.w);
	// CSM
	float4x4 sun_view_matrix;				//: packoffset(c9);
	float4x4 sun_cascade_projections[3];	//: packoffset(c14);
	float sunCascade0EndDepth;				//: packoffset(c26);
	float sunCascade1EndDepth;				//: packoffset(c26.y);
};

SamplerState sampler_linear					: register(s0);
SamplerState sampler_ansio					: register(s1);
// G-Buffers
Texture2D gbuffer_0							: register(t0);
Texture2D gbuffer_1							: register(t1);
Texture2D gbuffer_2							: register(t2);
// IBL	
TextureCube irradiance_map					: register(t3);
TextureCube prefiltered_env_map				: register(t4);
Texture2D<float2> brdf_lut					: register(t5);
// CSM	
SamplerComparisonState shadow_sampler		: register(s1);
Texture2DArray sun_cascaded_shadow_maps		: register(t6);

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
		Layout :	Rgba16f		|				  base color					|  ambi occl	|
					Rgba16f		|		 encoded normal			|  metalness	|  roughness	|
					D24S8		|				  hardware depth				|  stencil		|
		-----------------------------------------------------------------------------------------------------	*/
	float3 base_color		= gbuffer_0.Sample(sampler_linear, input.tex_coord).rgb;
	float ambient_occlusion	= gbuffer_0.Sample(sampler_linear, input.tex_coord).a;
	float2 encoded_normal	= gbuffer_1.Sample(sampler_linear, input.tex_coord).rg;
	float metalness			= gbuffer_1.Sample(sampler_linear, input.tex_coord).b;
	float roughness			= gbuffer_1.Sample(sampler_linear, input.tex_coord).a;
	float depth				= gbuffer_2.Sample(sampler_linear, input.tex_coord).r;	
	// opacity
	/*	=====================================================================================================
			Reconstructions from G-Buffer
		=====================================================================================================	*/
	float3 normal = DecodeNormal_StereographicProjection(encoded_normal);
	// -----------------------------------------------------------------------------------------------------
	float linear_depth = camera_projection_b / (depth - camera_projection_a);
	// -----------------------------------------------------------------------------------------------------
	float4 position_clip_space = float4(input.tex_coord * 2 - 1, depth, 1);
	position_clip_space.y *= -1;									
	float4 position_view_space = mul(position_clip_space, inverse_projection_matrix);		// try getting v from view_space diff
	position_view_space /= position_view_space.w;
	float3 position_world_space = mul(position_view_space, inverse_view_matrix).xyz;
	/*	=====================================================================================================
			Sun's Cascaded Shadow
		=====================================================================================================	*/
	// Cascade selection
	int cascade_i = 2;
	if (linear_depth < sunCascade0EndDepth) { cascade_i = 0; }
	else if (linear_depth < sunCascade1EndDepth) { cascade_i = 1;}
	// Calculating shadow map uv
	float4 position_sun_shadow_clip_space = mul(mul(float4(position_world_space, 1.0f), sun_view_matrix), sun_cascade_projections[cascade_i]);
	float2 sun_shadow_tex_coord = position_sun_shadow_clip_space.xy / position_sun_shadow_clip_space.w * 0.5f + 0.5f;
	sun_shadow_tex_coord.y = 1 - sun_shadow_tex_coord.y;
	float depth_from_sun = position_sun_shadow_clip_space.z / position_sun_shadow_clip_space.w;
	// Test for shadowing
	float sun_shadow_factor = sun_cascaded_shadow_maps.SampleCmp(shadow_sampler, float3(sun_shadow_tex_coord, cascade_i), depth_from_sun);
	
	/*	=====================================================================================================
			Lighting
		=====================================================================================================	*/	/*
			Pre - Calculations
		-----------------------------------------------------------------------------------------------------	*/
	float3 F0 = { 0.04f, 0.04f, 0.04f };	// specular reflectance at normal incidence (~0.02-0.08 for non-metals, baseColor for metals)
	F0 = lerp(F0, base_color, metalness);
	float3 l = normalize(-dL1.direction);			// light direction	//float3 l = normalize(lightPos - worldPos);
	float3 v = normalize(camera_position_world_space - position_world_space);	// view direction
	float3 h = normalize(l + v);												// halfway (between l and v)
	float3 n = normalize(normal);												// normal direction
	float3 r = normalize(reflect(v, n));										// reflection of v across n
	float VDotH = saturate(max(dot(v, h), 0));
	float NDotL = saturate(max(dot(n, l), 0.0001));
	float NDotV = saturate(max(dot(n, v), 0.0001));
	float NDotH = saturate(max(dot(n, h), 0.0001));
	// -----------------------------------------------------------------------------------------------------
	float irradiance = 2.0f;	// if point/spot apply falloff to irradiance
	float3 F = 0.0f;			// fresnel to copy (out) from specular function
	/*	-----------------------------------------------------------------------------------------------------
			BRDF Calculations
		-----------------------------------------------------------------------------------------------------	*/
	float3 direct_diffuse = DirectDiffuseBRDF_Lambert(base_color, irradiance, NDotL);
	float3 direct_specular = 0 * DirectSpecularBRDF_CookTorrence(roughness, F0, VDotH, NDotL, NDotV, NDotH, F);
	float3 indirect_diffuse = irradiance_map.SampleLevel(sampler_linear, n, 0).xyz * base_color;
	float3 indirect_specular = ApproximateSpecularIBL(prefiltered_env_map, brdf_lut, sampler_ansio, F0, roughness, r, NDotV);
	/*	-----------------------------------------------------------------------------------------------------
			Conservation of Energy and Shadowing
		-----------------------------------------------------------------------------------------------------	*/
	//float3 ks = F;	// fresnel already applied in specular components
	float3 kd = (1.0f - F) * (1.0f - metalness);
	float3 diffuse = direct_diffuse /** sun_shadow_factor*/ + indirect_diffuse;
	float3 specular = direct_specular /** sun_shadow_factor*/ + indirect_specular;
	float3 lighting = kd * diffuse + /*ks **/ specular;
	float3 final_color = lighting * ambient_occlusion;	// Ambient occlusion is too faint when applied only indirect lighting.

	/*	=====================================================================================================
			CSM Selection Visualization Debugging
		=====================================================================================================	*/
	/*
	if (csmDebug == 1) {  }
	if (cascade_i == 0) { final_color *= float3(1, 0.75, 0.75); }
	if (cascade_i == 1) { final_color *= float3(0.75, 1, 0.75); }
	if (cascade_i == 2) { final_color *= float3(0.75, 0.75, 1); }
	*/

	return float4(final_color, 1.0f);
}