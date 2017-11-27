#pragma once

#include "Component.h"
#include "..\Systems\Graphics\Material.h"
#include "..\Systems\Graphics\Mesh.h"

class DrawComponent : public Component
{
public:
	DrawComponent();
	DrawComponent(std::shared_ptr<Mesh>& _mesh, std::shared_ptr<Material>& _material);
	~DrawComponent();

	Mesh* GetMesh();
	Material* GetMaterial();

private:

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
};

