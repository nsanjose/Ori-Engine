#pragma once

#include <memory>

#include "..\..\Components\CameraComponent.h"
#include "..\Scenes\Entity.h"

class CameraSystem
{
public:
	CameraSystem();
	~CameraSystem();

	void Update();

	void UpdateViewMatrix(Entity& pr_camera);
	void UpdateProjectionMatrix(Entity& pr_camera, float p_aspect_ratio);
	void MoveRelative(Entity & pr_camera, float x, float y, float z);
	void Rotate(Entity & pr_camera, float x, float y);

	void AdjustSpeed(float difference);	// reset on camera swap?

private:
	float m_move_scale = 5;
};

