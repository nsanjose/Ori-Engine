#pragma once

#include <DirectXMath.h>

#include "Component.h"

class TransformComponent : public Component
{
public:
	TransformComponent();
	TransformComponent(DirectX::XMFLOAT3 p_position);
	TransformComponent(DirectX::XMFLOAT3 p_position, DirectX::XMFLOAT3 p_rotation);
	TransformComponent(DirectX::XMFLOAT3 p_position, DirectX::XMFLOAT3 p_rotation, DirectX::XMFLOAT3 p_scale);
	~TransformComponent();

	void UpdateWorldMatrix();	// dirty flag?
	DirectX::XMFLOAT4X4 GetWorldMatrix();

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 rotation;
	DirectX::XMFLOAT3 scale;
	
private:
	DirectX::XMFLOAT4X4 world_matrix;
};

