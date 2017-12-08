#include "TransformComponent.h"

using namespace DirectX;

TransformComponent::TransformComponent() : Component()
{
	m_position	= XMFLOAT3(0, 0, 0);
	m_rotation	= XMFLOAT3(0, 0, 0);
	m_scale		= XMFLOAT3(1, 1, 1);
	UpdateWorldMatrix();
}

TransformComponent::TransformComponent(XMFLOAT3 p_position) : Component()
{
	m_position	= p_position;
	m_rotation	= XMFLOAT3(0, 0, 0);
	m_scale		= XMFLOAT3(1, 1, 1);
	UpdateWorldMatrix();
}

TransformComponent::TransformComponent(XMFLOAT3 p_position, XMFLOAT3 p_rotation) : Component()
{
	m_position	= p_position;
	m_rotation	= p_rotation;
	m_scale		= XMFLOAT3(1, 1, 1);
	UpdateWorldMatrix();
}

TransformComponent::TransformComponent(XMFLOAT3 p_position, XMFLOAT3 p_rotation, XMFLOAT3 p_scale) : Component()
{
	m_position	= p_position;
	m_rotation	= p_rotation;
	m_scale		= p_scale;
	UpdateWorldMatrix();
}

TransformComponent::~TransformComponent()
{
}

void TransformComponent::UpdateWorldMatrix()
{
	if (m_is_world_matrix_dirty)
	{
		XMMATRIX translation_matrix = XMMatrixTranspose(XMMatrixTranslationFromVector(XMLoadFloat3(&m_position)));
		XMMATRIX rotation_matrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&m_rotation));
		XMMATRIX scale_matrix = XMMatrixScalingFromVector(XMLoadFloat3(&m_scale));

		XMMATRIX temp_world_matrix = translation_matrix * rotation_matrix * scale_matrix;
		XMStoreFloat4x4(&m_world_matrix, (temp_world_matrix));

		m_is_world_matrix_dirty = false;
	}
}
