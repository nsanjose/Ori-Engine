#include "CameraComponent.h"

using namespace DirectX;

CameraComponent::CameraComponent() : Component()
{
	xRotation = 0;
	yRotation = 0;
	XMStoreFloat4(&quaternion, XMQuaternionIdentity());		// handle initial rotation from entity transform? remove extraneous rotation vars
	XMStoreFloat4x4(&m_view_matrix, XMMatrixIdentity());
	m_near_clip = 0.1f;
	m_far_clip = 100.0f;
	XMStoreFloat4x4(&m_projection_matrix, XMMatrixIdentity());		// replace this and previous matricies set with identity to local update funcs? or keep logic in system
}

CameraComponent::CameraComponent(TransformComponent& pr_transform_component, float p_fov, float p_aspect_ratio, float p_near_clip, float p_far_clip) : Component()
{
	// View
	XMStoreFloat4(&quaternion, XMQuaternionRotationRollPitchYaw(pr_transform_component.m_rotation.x, pr_transform_component.m_rotation.y, 0));

	XMVECTOR camera_direction = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMLoadFloat4(&quaternion));
	XMStoreFloat3(&forward, camera_direction);

	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&pr_transform_component.m_position),	// camera position
		camera_direction,									// camera direction
		XMVectorSet(0, 1, 0, 0)								// camera's up direction
	);
	XMStoreFloat4x4(&m_view_matrix, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_inverse_view, XMMatrixTranspose(XMMatrixInverse(nullptr, view)));

	// Projection
	m_fov = p_fov;
	m_aspect_ratio = p_aspect_ratio;
	m_near_clip = p_near_clip;
	m_far_clip = p_far_clip;
	XMMATRIX proj = XMMatrixPerspectiveFovLH(
		m_fov,				// Field of View Angle (default 0.25f * XM_PI)
		m_aspect_ratio,		// Aspect ratio
		m_near_clip,		// Near clip plane distance
		m_far_clip);		// Far clip plane distance
	XMStoreFloat4x4(&m_projection_matrix, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_inverse_projection_matrix, XMMatrixTranspose(XMMatrixInverse(nullptr, proj)));

	m_projection_a = m_far_clip / (m_far_clip - m_near_clip);
	m_projection_b = (-m_far_clip * m_near_clip) / (m_far_clip - m_near_clip);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invVP = XMMatrixInverse(nullptr, viewProj);
	XMStoreFloat4x4(&m_inverse_view_projection_matrix, XMMatrixTranspose(invVP));

}

CameraComponent::~CameraComponent()
{
}

const float& CameraComponent::GetFov() const
{
	return m_fov;
}

const float& CameraComponent::GetAspectRatio() const
{
	return m_aspect_ratio;
}

const float& CameraComponent::GetNearClip() const
{
	return m_near_clip;
}

const float& CameraComponent::GetFarClip() const
{
	return m_far_clip;
}
