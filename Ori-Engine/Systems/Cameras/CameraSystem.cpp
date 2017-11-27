#include "CameraSystem.h"

#include <Windows.h>

using namespace DirectX;

CameraSystem::CameraSystem()
{
}

CameraSystem::~CameraSystem()
{
}

void CameraSystem::Update()
{
	// accept events?
}

void CameraSystem::MoveRelative(Entity& pr_camera, float p_difference_x, float p_difference_y, float p_difference_z)
{
	TransformComponent& transform_component = pr_camera.GetTransformComponent();
	CameraComponent* camera_component = pr_camera.GetComponentByType<CameraComponent>();

	// relativise desired direction
	XMVECTOR movement = XMVector3Rotate(XMVectorSet(p_difference_x, p_difference_y, p_difference_z, 0), XMLoadFloat4(&camera_component->quaternion));
	XMStoreFloat3(&transform_component.position, XMLoadFloat3(&transform_component.position) + (movement * m_move_scale));

	UpdateViewMatrix(pr_camera);
}

void CameraSystem::Rotate(Entity& pr_camera, float p_difference_x, float p_difference_y)
{
	TransformComponent& transform_component = pr_camera.GetTransformComponent();
	CameraComponent* camera_component = pr_camera.GetComponentByType<CameraComponent>();

	// Adjust the current rotation
	transform_component.rotation.x += p_difference_x * .005f;
	transform_component.rotation.y += p_difference_y * .005f;

	// Clamp the x between PI/2 and -PI/2
	transform_component.rotation.x = max(min(transform_component.rotation.x, XM_PIDIV2), -XM_PIDIV2);

	// Update quaternion
	XMStoreFloat4(&camera_component->quaternion, XMQuaternionRotationRollPitchYaw(transform_component.rotation.x, transform_component.rotation.y, 0));

	UpdateViewMatrix(pr_camera);
}

void CameraSystem::AdjustSpeed(float difference)
{
	m_move_scale *= 1 + (difference * .05f);
}

void CameraSystem::UpdateViewMatrix(Entity& pr_camera)
{
	TransformComponent transform_component = pr_camera.GetTransformComponent();
	CameraComponent* camera_component = pr_camera.GetComponentByType<CameraComponent>();

	// new direction = standard "forward" matrix rotated by our rotation
	XMVECTOR camera_direction = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMLoadFloat4(&camera_component->quaternion));
	XMStoreFloat3(&camera_component->forward, camera_direction);

	// update view matrix
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&transform_component.position),	// camera position
		camera_direction,								// camera direction
		XMVectorSet(0, 1, 0, 0)							// camera's up axis
	);
	XMStoreFloat4x4(&camera_component->m_view_matrix, XMMatrixTranspose(view));

	XMStoreFloat4x4(&camera_component->m_inverse_view, XMMatrixTranspose(XMMatrixInverse(nullptr, view)));

	XMMATRIX temp_projection_matrix = XMMatrixTranspose(XMLoadFloat4x4(&camera_component->m_projection_matrix));
	XMMATRIX temp_view_projection_matrix = XMMatrixMultiply(view, temp_projection_matrix);
	XMMATRIX temp_inverse_view_projection_matrix = XMMatrixInverse(nullptr, temp_view_projection_matrix);
	XMStoreFloat4x4(&camera_component->m_inverse_view_projection_matrix, XMMatrixTranspose(temp_inverse_view_projection_matrix));
}

void CameraSystem::UpdateProjectionMatrix(Entity& pr_camera, float p_aspect_ratio)
{
	CameraComponent* camera_component = pr_camera.GetComponentByType<CameraComponent>();
	float near_clip = camera_component->GetNearClip();	// all projection matrix constructor parameters should be new/passed? (none from current camera unless specified)
	float far_clip = camera_component->GetFarClip();

	XMMATRIX temp_projection_matrix = XMMatrixPerspectiveFovLH(
		0.25f * XM_PI,		// Field of View Angle
		p_aspect_ratio,		// Aspect ratio
		near_clip,			// Near clip plane distance
		far_clip);			// Far clip plane distance
	XMStoreFloat4x4(&camera_component->m_projection_matrix, XMMatrixTranspose(temp_projection_matrix));

	camera_component->m_projection_a = far_clip / (far_clip - near_clip);	// access elements _34, _43 instead of calculating
	camera_component->m_projection_b = (-far_clip * near_clip) / (far_clip - near_clip);

	XMStoreFloat4x4(&camera_component->m_inverse_projection_matrix, XMMatrixTranspose(XMMatrixInverse(nullptr, temp_projection_matrix)));
}