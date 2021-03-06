/*	=====================================================================================================
		Pixel Shader
		Bake BRDF LUT for PBR
	=====================================================================================================	*/
#include "common_pbr.hlsl"

struct QuadVsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
};

float2 IntegrateBRDF(float Roughness, float NoV)
{
	float3 N = float3(0.0f, 0.0f, 1.0f);
	float3 V;
	V.x = sqrt(1.0f - NoV * NoV); // sin
	V.y = 0;
	V.z = NoV; // cos
	float A = 0;
	float B = 0;
	const uint NumSamples = 1024;
	for (uint i = 0; i < NumSamples; i++)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, Roughness, N);
		float3 L = 2 * dot(V, H) * H - V;
		float NoL = saturate(L.z);
		float NoH = saturate(H.z);
		float VoH = saturate(dot(V, H));
		if (NoL > 0)
		{
			float G = G_Smith(Roughness, NoV, NoL);
			float G_Vis = G * VoH / (NoH * NoV);
			float Fc = pow(1 - VoH, 5);
			A += (1 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}
	return float2(A, B) / NumSamples;
}

float2 main(QuadVsOut input) : SV_TARGET
{
	return IntegrateBRDF(input.tex_coord.x, input.tex_coord.y);
}