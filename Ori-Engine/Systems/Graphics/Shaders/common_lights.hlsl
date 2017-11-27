/*	=====================================================================================================
		Common
		Lights
	=====================================================================================================	*/

struct DirectionalLight	// color, direction
{
	float4 ambientColor;
	float4 diffuseColor;
	float3 direction;
};

struct PointLight	// color, location, radius
{
	float4 ambientColor;
	float4 diffuseColor;
	float3 location;
};

struct SpotLight	// color, location, angle, direction
{
	float4 ambientColor;
	float4 diffuseColor;
	float3 location;
	float angle;
	float3 direction;
};

uint IsInSpotLight(float3 surface_direction_to_light, float3 light_direction, float light_angle)
{
	float angle_to_surface = acos(dot(-surface_direction_to_light, light_direction));	// acos on cpu?
	return step(angle_to_surface, light_angle);
}

float PointLightFalloff_UE4(float3 lightPosition, float3 surfacePosition, float lightRadius)
{
	float distance = length(lightPosition - surfacePosition);
	return pow(saturate(1 - pow(distance / lightRadius, 4)), 2)
		/ (pow(distance, 2) + 1);
}

float shadowCalc(Texture2D shadowMap, SamplerComparisonState shadowSampler, float4 shadowPos)
{
	// Convert NDC to UV coordinates (flip?)
	float2 shadowUV = shadowPos.xy / shadowPos.w * 0.5f + 0.5f;
	shadowUV.y = 1.0f - shadowUV.y;
	float depthFromLight = shadowPos.z / shadowPos.w;
	//depthFromLight -= .001f;
	return shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV, depthFromLight);
}