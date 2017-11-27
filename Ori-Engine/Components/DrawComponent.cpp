#include "DrawComponent.h"

DrawComponent::DrawComponent() : Component()
{
}

DrawComponent::DrawComponent(std::shared_ptr<Mesh>& _mesh, std::shared_ptr<Material>& _material) : Component()
{
	mesh = _mesh;
	material = _material;
}

DrawComponent::~DrawComponent()
{
}

Mesh* DrawComponent::GetMesh()
{
	return mesh.get();
}

Material* DrawComponent::GetMaterial()
{
	return material.get();
}