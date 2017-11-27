#include "TransformComponent.h"

using namespace DirectX;

TransformComponent::TransformComponent() : Component()
{
	position = XMFLOAT3(0, 0, 0);
	rotation = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);
	world_matrix = GetWorldMatrix();
}

TransformComponent::TransformComponent(XMFLOAT3 p_position) : Component()
{
	position = p_position;
	rotation = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);
	world_matrix = GetWorldMatrix();
}

TransformComponent::TransformComponent(XMFLOAT3 p_position, XMFLOAT3 p_rotation) : Component()
{
	position = p_position;
	rotation = p_rotation;
	scale = XMFLOAT3(1, 1, 1);
	world_matrix = GetWorldMatrix();
}

TransformComponent::TransformComponent(XMFLOAT3 p_position, XMFLOAT3 p_rotation, XMFLOAT3 p_scale) : Component()
{
	position = p_position;
	rotation = p_rotation;
	scale = p_scale;
	world_matrix = GetWorldMatrix();
}

TransformComponent::~TransformComponent()
{
}

void TransformComponent::UpdateWorldMatrix()
{
	XMMATRIX translation_matrix = XMMatrixTranspose(XMMatrixTranslationFromVector(XMLoadFloat3(&position)));
	XMMATRIX rotation_matrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
	XMMATRIX scale_matrix = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

	XMMATRIX temp_world_matrix = translation_matrix * rotation_matrix * scale_matrix;

	XMStoreFloat4x4(&world_matrix, (temp_world_matrix));
}

XMFLOAT4X4 TransformComponent::GetWorldMatrix()
{
	UpdateWorldMatrix();
	return world_matrix;
}


