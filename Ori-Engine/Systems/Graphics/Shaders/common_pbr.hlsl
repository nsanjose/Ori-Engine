/*	=====================================================================================================
		Common
		Physically Based Rendering (PBR)
	=====================================================================================================	*/

static const float PI = 3.141592654f;

/*	=====================================================================================================
		BRDF Components for Direct Lighting
	=====================================================================================================
		F - Fresnel
	-----------------------------------------------------------------------------------------------------	*/
float3 F_Schlick_UE4(float VDotH, float3 F0)	//UE4's Spherical Gaussian approximation
{
	return F0 + (1.0f - F0) * pow((1.0f - VDotH), 5.0f);
	//return F0 + (1.0f - F0) * pow(2, (-5.55473f * VDotH - 6.98316) * VDotH);
}
/*	-----------------------------------------------------------------------------------------------------
		D - Distribution (NDF)
	-----------------------------------------------------------------------------------------------------	*/
float D_TrowbridgeReitz_GGX(float NDotH, float roughness)	//Disney's reparameterization of a=roughness^2
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NDotH2 = NDotH * NDotH;
	float denominator_part = (NDotH2 * (a2 - 1.0f)) + 1.0f;
	return a2 / (PI * denominator_part * denominator_part);
}
/*	-----------------------------------------------------------------------------------------------------
		G - Geometric Shadowing
	-----------------------------------------------------------------------------------------------------	*/
float G_Schlick_GGX_Disney(float NDotV, float roughness)	//Disney's modification of k -> ((k + 1) / 2) to reduce hotness in non-analytical lights
{
	float k = ((roughness + 1) * (roughness + 1)) / 8;
	return NDotV / (NDotV * (1 - k) + k);
}
float G_Smith_Disney(float roughness, float NDotV, float NDotL)
{
	float GSL = G_Schlick_GGX_Disney(NDotL, roughness);
	float GSV = G_Schlick_GGX_Disney(NDotV, roughness);
	return GSL * GSV;
}
float G_Schlick_GGX(float NDotV, float roughness) // Not Disney modified, for IBL
{
	float k = (roughness * roughness) / 2;
	return NDotV / (NDotV * (1 - k) + k);
}
float G_Smith(float roughness, float NDotV, float NDotL)
{
	float GSL = G_Schlick_GGX(NDotL, roughness);
	float GSV = G_Schlick_GGX(NDotV, roughness);
	return GSL * GSV;
}
/*	=====================================================================================================
		Direct Lighting	
		https://en.wikipedia.org/wiki/Lambertian_reflectance
		https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
	=====================================================================================================	*/
float3 DirectDiffuseBRDF_Lambert(float3 base_color, float irradiance, float NDotL)
{
	return ((base_color * irradiance) * NDotL / PI);
}
float3 DirectSpecularBRDF_CookTorrence(float roughness, float3 F0, float VDotH, float NDotL, float NDotV, float NDotH, out float3 ks)
{
	float3 F = F_Schlick_UE4(VDotH, F0);					// Fresnel
	float D = D_TrowbridgeReitz_GGX(NDotH, roughness);		// Normal Distribution Function
	float G = G_Smith(roughness, NDotV, NDotL);				// Geometric Shadowing
	
	ks = F;	// output for Conservation of Energy (applying also to diffuse)

	return (F * G * D) / (4 * NDotL * NDotV);
}

/*	=====================================================================================================
		Image Based (Indirect) Lighting	
		https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
	=====================================================================================================
		Indirect Specular - UE4
	-----------------------------------------------------------------------------------------------------	*/
static const int PFEM_MAX_MIP_LEVEL = 6;
float3 ApproximateSpecularIBL(TextureCube pfem, Texture2D<float2> brdf_lut, SamplerState sampler_ansio, float3 specular_color, float roughness, float3 r, float NDotV)
{
	int mip_i = roughness * PFEM_MAX_MIP_LEVEL;
	float3 prefiltered_color = pfem.SampleLevel(sampler_ansio, r, mip_i).rgb;
	float2 brdf_lut_color = brdf_lut.Sample(sampler_ansio, float2(roughness, NDotV));

	return prefiltered_color * (specular_color * brdf_lut_color.x + brdf_lut_color.y);
}
/*	-----------------------------------------------------------------------------------------------------
//  IBL Probe Baking (PFEM + BRDF LUT) 
	-----------------------------------------------------------------------------------------------------	*/
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float2 Hammersley(uint i, uint N)
{
	float radical_inverse_VanDerCorput = reversebits(i) * 2.3283064365386963e-10f;
	return float2(float(i) / float(N), radical_inverse_VanDerCorput);
}
float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
	float a = Roughness * Roughness;

	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);

	float3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;

	float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 TangentX = normalize(cross(UpVector, N));
	float3 TangentY = cross(N, TangentX);

	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}
