/*	=====================================================================================================
		Pixel Shader
		Bake Prefiltered Environment Map for PBR
	=====================================================================================================	*/
#include "common_pbr.hlsl"

cbuffer BufferParameters	: register(b0)
{
	int face_i;
	int mip_i;
}

TextureCube env_map				: register(t0);
SamplerState sampler_ansio		: register(s0);

struct QuadVsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
};

float3 NormalFromTexCoord(int targetFace, float2 tex_coord)
{
	// rescale from [0, 1] to [-1, 1]
	float2 rescaled = tex_coord * 2.0f - 1.0f;
	// reorient towards target face
	float3 normal = 0;
	switch (targetFace)
	{
		case 0:	// right
			normal = float3(1.0f, rescaled.y, rescaled.x);
			break;
		case 1: // left
			normal = float3(-1.0f, rescaled.y, -rescaled.x);
			break;
		case 2:	// top
			normal = float3(rescaled.x, -1.0f, -rescaled.y);
			break;
		case 3:	// bottom
			normal = float3(rescaled.x, 1.0f, rescaled.y);
			break;
		case 4:	// front
			normal = float3(rescaled.x, rescaled.y, -1.0f);
			break;
		case 5:	// back
			normal = float3(-rescaled.x, rescaled.y, 1.0f);
			break;
	}
	return normal;
}

float3 PrefilterEnvMap(float Roughness, float3 R)
{
	float TotalWeight = 0.0000001f;

	float3 N = R;
	float3 V = R;
	float3 PrefilteredColor = 0;

	const uint NumSamples = 1024;

	for (uint i = 0; i < NumSamples; i++)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, Roughness, N);
		float3 L = 2 * dot(V, H) * H - V;
		float NoL = saturate(dot(N, L));

		if (NoL > 0)
		{
			PrefilteredColor += env_map.SampleLevel(sampler_ansio, L, 0).rgb * NoL;
			TotalWeight += NoL;
		}
	}

	return PrefilteredColor / TotalWeight;
}

float4 main(QuadVsOut input) : SV_TARGET
{
	float3 normal = NormalFromTexCoord(face_i, input.tex_coord);
	float roughness = saturate(mip_i / 6.0f);

	return float4(PrefilterEnvMap(roughness, normal), 1);
}