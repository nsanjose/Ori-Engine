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

	DirectX::XMFLOAT3 m_position;
	DirectX::XMFLOAT3 m_rotation;
	DirectX::XMFLOAT3 m_scale;

	bool m_is_world_matrix_dirty;

	void UpdateWorldMatrix();
	DirectX::XMFLOAT4X4 m_world_matrix;
};

