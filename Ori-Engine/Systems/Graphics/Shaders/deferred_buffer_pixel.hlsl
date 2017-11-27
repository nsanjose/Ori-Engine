/*	=====================================================================================================
		Pixel Shader
		Deferred Shading
		First Pass: Populate GBuffers with data
	=====================================================================================================	*/
#include "common_deferred.hlsl"

SamplerState sampler_filtering_choice	: register(s0);
Texture2D base_color_map				: register(t0);
Texture2D metalness_map					: register(t1);
Texture2D roughness_map					: register(t2);
Texture2D normal_map					: register(t3);
Texture2D ambient_occlusion_map			: register(t4);

struct VsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
};

struct PsOut {
	float4 texture_one	: SV_TARGET0;
	float4 texture_two	: SV_TARGET1;
};

PsOut main(VsOut input)
{
	// Normal in Tangent -> World Conversion
	float3 normal_from_map = normal_map.Sample(sampler_filtering_choice, input.tex_coord).rgb * 2 - 1;
	input.tangent = normalize(input.tangent);
	// Calculate the TBN matrix
	float3 N = normalize(input.normal);
	float3 T = normalize(input.tangent - N * dot(input.tangent, N));
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);
	input.normal = normalize(mul(normal_from_map, TBN));
	float2 encoded_normal = EncodeNormal_StereographicProjection(input.normal);
	
	float3 base_color		= base_color_map.Sample(sampler_filtering_choice, input.tex_coord).rgb;
	float metalness			= metalness_map.Sample(sampler_filtering_choice, input.tex_coord).r;
	float roughness			= roughness_map.Sample(sampler_filtering_choice, input.tex_coord).r;
	float ambient_occlusion	= ambient_occlusion_map.Sample(sampler_filtering_choice, input.tex_coord).r;

	PsOut output;
	output.texture_one = float4(base_color, ambient_occlusion);
	output.texture_two = float4(encoded_normal, metalness, roughness);
	return output;
}