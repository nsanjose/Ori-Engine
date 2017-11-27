#pragma once

#include <DirectXMath.h>

#include "Component.h"
#include "TransformComponent.h"

class CameraComponent : public Component
{
public:
	CameraComponent();
	CameraComponent(TransformComponent& pr_transform_component, float p_fov, float p_aspect_ratio, float p_near_clip, float p_far_clip);
	~CameraComponent();

	const float& GetFov() const;
	const float& GetAspectRatio() const;
	const float& GetNearClip() const;
	const float& GetFarClip() const;

	float xRotation;		// replace with entity transform rotation
	float yRotation;
	DirectX::XMFLOAT4 quaternion;

	DirectX::XMFLOAT4X4 m_view_matrix;
	DirectX::XMFLOAT4X4 m_projection_matrix;
	// For reconstruction of world space position in deferred compilation shader
	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT4X4 m_inverse_projection_matrix;
	DirectX::XMFLOAT4X4 m_inverse_view_projection_matrix;
	float m_projection_a;
	float m_projection_b;
	// For cascade frustum splitting
	DirectX::XMFLOAT4X4 m_inverse_view;

private:
	float m_fov;
	float m_aspect_ratio;
	float m_near_clip;
	float m_far_clip;
};

