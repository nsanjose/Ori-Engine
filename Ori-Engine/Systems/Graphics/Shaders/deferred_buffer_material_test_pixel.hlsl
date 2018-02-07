/*	=====================================================================================================
		Pixel Shader
		Deferred Shading
		First Pass: Populate GBuffers with data
	=====================================================================================================	*/
#include "common_deferred.hlsl"

cbuffer BufferParameters : register(b0)
{
	float3 base_color;
	float metalness;
	float roughness;
};

struct VsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
};

struct PsOut
{
	float4 texture_one		: SV_TARGET0;
	float4 texture_two		: SV_TARGET1;
	float4 texture_three	: SV_TARGET2;
};

PsOut main(VsOut input)
{
	float ambient_occlusion	= 1.0f;

	float3 normal = float3(0.0f, 0.0f, 1.0f);
	input.tangent = normalize(input.tangent);
	// Calculate the TBN matrix
	float3 N = normalize(input.normal);
	float3 T = normalize(input.tangent - N * dot(input.tangent, N));
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);
	input.normal = normalize(mul(normal, TBN));
	float2 encoded_normal = EncodeNormal_StereographicProjection(input.normal);
	
	PsOut output;
	output.texture_one = float4(base_color, 0);
	output.texture_two = float4(encoded_normal, metalness, roughness);
	output.texture_three = float4(0, 0, 0, ambient_occlusion);
	return output;
}