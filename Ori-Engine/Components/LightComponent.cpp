#include "LightComponent.h"

LightComponent::LightComponent() : Component()
{
}

LightComponent::LightComponent(std::unique_ptr<Light> p_light) : Component()
{
	light = std::move(p_light);
}

LightComponent::~LightComponent()
{
}

Light* LightComponent::GetGenericLight() {
	return light.get();
}