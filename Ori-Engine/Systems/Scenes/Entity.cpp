#include "Entity.h"

Entity::Entity()
{
	components.push_back(std::move(std::make_unique<TransformComponent>()));
}

Entity::Entity(std::unique_ptr<TransformComponent> pup_transform_component)
{
	components.push_back(std::move(pup_transform_component));
}

Entity::~Entity()
{
}

Component& Entity::AddComponent(std::unique_ptr<Component> pup_new_component)
{
	components.push_back(std::move(pup_new_component));
	return *pup_new_component.get();
}

TransformComponent& Entity::GetTransformComponent() const
{
	TransformComponent* transform = static_cast<TransformComponent*>(components[0].get());
	return *transform;
}
