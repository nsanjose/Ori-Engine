#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <vector>

#include "..\Core\D3D11Renderer.h"
#include "..\Systems\Cameras\CameraSystem.h"
#include "..\Systems\Graphics\GraphicsSystem.h"
#include "..\Systems\Input\InputSystem.h"
#include "..\Systems\Scenes\SceneSystem.h"

class Framework
	: public D3D11Renderer
{
public:
	Framework(HINSTANCE p_hinstance);
	~Framework();

	// HRESULT returns
	void Initialize();
	void Update(float p_delta_time, float p_total_time);
	void Draw(float p_delta_time, float p_total_time);

	//void OnResize();

private:
	std::unique_ptr<SceneSystem> mup_scene_manager;
	std::unique_ptr<CameraSystem> mup_camera_system;
	std::unique_ptr<GraphicsSystem> mup_graphics_system;
	std::unique_ptr<InputSystem> mup_input_system;
};

