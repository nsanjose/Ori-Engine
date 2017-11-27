#include "LightComponent.h"

LightComponent::LightComponent() : Component()
{
}

LightComponent::LightComponent(std::shared_ptr<Light> _light) : Component()
{
	light = _light;
}

LightComponent::~LightComponent()
{
}

Light* LightComponent::GetGenericLight() {
	return light.get();
}

std::shared_ptr<Shadow>& LightComponent::GetShadow()
{
	if (!shadow)
	{
		shadow = std::make_shared<Shadow>();
	}
	return shadow;
}
