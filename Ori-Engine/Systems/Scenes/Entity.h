#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "..\..\Components\Component.h"
#include "..\..\Components\TransformComponent.h"

class Entity
{
public:
	Entity();
	Entity(std::unique_ptr<TransformComponent> pup_transform_component);
	~Entity();

	Component& AddComponent(std::unique_ptr<Component> pup_new_component);
	TransformComponent& GetTransformComponent() const;

	template<class T>
	bool HasComponent()
	{
		const std::type_info& search_typeid = typeid(T);
		for (int i = 0; i < components.size(); i++)
		{
			auto c = components[i].get();
			if (c && typeid(*c) == search_typeid)
			{
				return true;
			}
		}
		return false;
	};

	template<class T>
	T* GetComponentByType() const
	{
		const std::type_info& search_typeid = typeid(T);
		for (int i = 1; i < components.size(); i++)
		{
			auto c = components[i].get();
			if (c && typeid(*c) == search_typeid)
			{
				return dynamic_cast<T*>(c);
			}
		}

		std::cout << "ERROR in Entity::GetComponentByType<T>() -- Entity does not have a component of provided type, T.\n";
		return nullptr;
	};

	template<class T>
	void RemoveComponentByType() const
	{
		const std::type_info& search_typeid = typeid(T);
		for (int i = 0; i < components.size(); i++)
		{
			auto c = components[i].get();
			if (c && typeid(*c) == search_typeid)
			{
				if (typeid(TransformComponent) == search_typeid)
				{
					std::cout << "Error in Entity::RemoveComponentByType<T>() -- Cannot remove transform component from entity.\n";
					return;
				}

				components.erase(components.begin() + i);
				return;
			}
		}

		std::cout << "ERROR in Entity::RemoveComponentByType<T>() -- Entity does not have a component of provided type, T.\n";
	};

private:
	std::vector<std::unique_ptr<Component>> components;
};

