/*	=====================================================================================================
		Common
		Deferred Rendering
	=====================================================================================================	*/

/*	=====================================================================================================
		Encoding Normals in G-Buffer https://aras-p.info/texts/CompactNormalStorage.html
	=====================================================================================================	*/	/*
		Stereographic Projection
	-----------------------------------------------------------------------------------------------------	*/
float2 EncodeNormal_StereographicProjection(float3 n)
{
	float scale = 1.7777;
	//if (n.z == -1) { n.z = .00000001f; }
	float2 eN = n.xy / (n.z + 1);
	eN /= scale;
	eN = eN * 0.5 + 0.5;
	return eN;
}
float3 DecodeNormal_StereographicProjection(float2 eN)
{
	float scale = 1.7777;
	float3 nn = float3(eN, 0) * float3(2 * scale, 2 * scale, 0) + float3(-scale, -scale, 1);
	float g = 2.0 / dot(nn.xyz, nn.xyz);
	float3 n;
	n.xy = g * nn.xy;
	n.z = g - 1;
	return n;
}
/*	-----------------------------------------------------------------------------------------------------
		Spherical Environment Mapping
	-----------------------------------------------------------------------------------------------------	*/
float2 EncodeNormal_SphereMap(float3 n)
{
	float2 en = normalize(n.xy) * sqrt(n.z * 0.5f + 0.5f);
	en = en * 0.5f + 0.5f;
	return en;
}
float3 DecodeNormal_SphereMap(float2 eN)
{
	float4 nn = float4(eN, 0, 0) * float4(2, 2, 0, 0) + float4(-1, -1, 1, -1);
	float l = dot(nn.xyz, -nn.xyw);
	nn.z = l;
	nn.xy = sqrt(l);
	return nn.xyz * 2 + float3(0, 0, -1);
}
float2 EncodeNormal_LaeaProjection(float3 n)
{
	if (n.z == -1) { n.z = -.9999999f; }
	float p = sqrt(n.z * 8 + 8);
	return float2(n.xy / p + 0.5);
}
float3 DecodeNormal_LaeaProjection(float2 eN)
{
	float2 feN = eN * 4 - 2;
	float f = dot(feN, feN);
	float g = sqrt(1 - f / 4);
	float3 n;
	n.xy = feN * g;
	n.z = 1 - f / 2;
	return n;
}
/*	-----------------------------------------------------------------------------------------------------
		CryEngine 3		http://www.crytek.com/sites/default/files/A_bit_more_deferred_-_CryEngine3.ppt	
	//heavy artifacting on normals >~70degrees??
	-----------------------------------------------------------------------------------------------------	*/
float2 EncodeNormal_Cry(float3 n)
{
	return normalize(n.xy) * sqrt(n.z * 0.5 + 0.5);
}
float3 DecodeNormal_Cry(float2 eN)
{
	float3 n;
	n.z = length(eN.xy) * 2 - 1;
	n.xy = normalize(eN.xy) * sqrt(1 - n.z * n.z);
	return n;
}
