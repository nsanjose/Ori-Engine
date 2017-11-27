/*	=====================================================================================================
		Common
		High Dynamic Range (HDR) Rendering
	=====================================================================================================	*/

/*	=====================================================================================================
		Luminance	
		https://www.w3.org/Graphics/Color/sRGB
	=====================================================================================================	*/
float LuminanceFromRgb(float3 color)
{
	return max(dot(color, float3(0.2126f, 0.7152f, 0.0722f)), 0.0001f);
}

/*	=====================================================================================================
		Eye Adaptive Exposure	
		https://www.academia.edu/24772316/Programming_Vertex_Geometry_and_Pixel_Shaders_Screenshots_of_Alan_Wake_courtesy_of_Remedy_Entertainment
	=====================================================================================================	*/
float AverageLuminanceFromMaxMip(SamplerState samplerPoint, Texture2D<float1> luminanceTex)
{
	return max(luminanceTex.Sample(samplerPoint, float2(0.5f, 0.5f)).r, 0.0001f);
}
float3 Expose(float3 color, float averageLuminance)
{
	float3 middleGrey = 1.03f - 2.0f / (2.0f + log10(averageLuminance + 1.0f));
	return color * middleGrey / averageLuminance;
}
/*	=====================================================================================================
		Tone Mapping Global Operators
	=====================================================================================================	*/	/*
		Reinhard	
		http://www.cmap.polytechnique.fr/~peyre/cours/x2005signal/hdr_photographic.pdf
	-----------------------------------------------------------------------------------------------------	*/
float3 ToneMap_Reinhard_Detail(float3 color)
{
	float luminance = LuminanceFromRgb(color);
	float toneMappedLuminance = luminance / (1 + luminance);
	return (color / luminance) * toneMappedLuminance;
}
float3 ToneMap_Reinhard_BurnControl(float3 color, float whiteThreshold)
{
	float luminance = LuminanceFromRgb(color);
	float toneMappedLuminance = luminance * (1.0f + luminance / (whiteThreshold * whiteThreshold)) / (1 + luminance);
	return (color / luminance) * toneMappedLuminance;
}
/*	-----------------------------------------------------------------------------------------------------
		Filmic - Uncharted 2	
		https://www.gdcvault.com/play/1012351/Uncharted-2-HDR
	-----------------------------------------------------------------------------------------------------	*/
float3 Uncharted2_Curve(float3 x)
{
	float A = 0.22f;	// Shoulder Strength
	float B = 0.30f;	// Linear Strength
	float C = 0.10f;	// Linear Angle
	float D = 0.20f;	// Toe Strength
	float E = 0.01f;	// Toe Numerator
	float F = 0.30f;	// Toe Denominator

	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
float3 ToneMap_Filmic_Uncharted2(float3 color)
{
	float3 linearWhite = 11.2f;

	return Uncharted2_Curve(color) / Uncharted2_Curve(linearWhite);
}