/*	=====================================================================================================
		Common
		Lights
	=====================================================================================================	*/

interface AbstractLight
{
	float3 GetColor();
	float GetIrradiance(float3 surface_position_ws);
	float3 GetDirectionToLight(float3 surface_position_ws);
};

class Light : AbstractLight
{
	float3 color;
	float irradiance;

	float3 GetColor() { return color; }
	float GetIrradiance(float3 surface_position_ws) { return irradiance; }
	float3 GetDirectionToLight(float3 surface_position_ws) { return 0; } // virtual
};

class Linked_DirectionalLight : Light
{
	float3 direction;

	float3 GetDirectionToLight(float3 surface_position_ws) { return -direction; }
};

// use volume stenciling instead
uint IsInSpotLight(float3 surface_direction_to_light, float3 light_direction, float light_angle)
{
	float angle_to_surface = acos(dot(-surface_direction_to_light, light_direction));	// acos on cpu?
	return step(angle_to_surface, light_angle);
}

class Linked_SpotLight : Light
{
	float3 position_ws;
	//float3 direction;
	//float angle;

	float3 GetDirectionToLight(float3 surface_position_ws) { return position_ws - surface_position_ws; }
};

float PointLightFalloff_UE4(float3 light_position, float light_radius, float3 surface_position)
{
	float distance = length(light_position - surface_position);
	return pow(saturate(1 - pow(distance / light_radius, 4)), 2) / (pow(distance, 2) + 1);
}

class Linked_PointLight : Light
{
	float3 position_ws;
	float radius;

	float GetIrradiance(float3 surface_position_ws) { return irradiance * PointLightFalloff_UE4(position_ws, radius, surface_position_ws); }
	float3 GetDirectionToLight(float3 surface_position_ws) { return position_ws - surface_position_ws; }
};


