#include "Framework.h"

#include "..\Systems\Scenes\DemoScenes\DemoScene.h"
#include "..\Systems\Scenes\DemoScenes\DemoScene2.h"
#include "..\Systems\Scenes\DemoScenes\DemoScene3.h"

using namespace DirectX;

Framework::Framework(HINSTANCE p_hinstance)
	: D3D11Renderer(
		p_hinstance,
		"Ori Engine",		// Window title
		1750,				// Window client width
		750)				// Window client height
{
	//
}

Framework::~Framework()
{
}

void Framework::Initialize()
{
	// Systems
	mup_scene_manager = std::make_unique<SceneSystem>();
	mup_camera_system = std::make_unique<CameraSystem>();
	mup_graphics_system = std::make_unique<GraphicsSystem>(mcp_device.Get(), mcp_context.Get(), mcp_swapchain.Get(), mcp_backbuffer_rtv.Get(), m_width, m_height);
	mup_input_system = std::make_unique<InputSystem>(*mup_scene_manager.get(), *mup_camera_system.get(), &m_hwnd);
	
	// Tools
	ImageBasedLightingBaker * ibl_baker = mup_graphics_system->GetImageBasedLightingBaker();
	ShadowRenderer* shadow_renderer = mup_graphics_system->GetShadowRenderer();

	/*
	DemoScene* demo_scene = new DemoScene(	// PBR, Particles, Bloom, Eye Adaptive Exposure, Tone Mapping
		mcp_device.Get(), mcp_context.Get(), m_width, m_height,
		ibl_baker, shadow_renderer);
	*/
	/*
	DemoScene2* demo_scene = new DemoScene2(	// CSM
		mcp_device.Get(), mcp_context.Get(), m_width, m_height,
		ibl_baker, shadow_renderer);
	*/
	/**/
	DemoScene3* demo_scene = new DemoScene3(	// SSAO
		mcp_device.Get(), mcp_context.Get(), m_width, m_height,
		ibl_baker, shadow_renderer);
	/**/

	mup_scene_manager->AddScene(demo_scene);
}

void Framework::Update(float p_delta_time, float p_total_time)
{
	// passing events instead of reference control in input system?
	mup_input_system->Update(p_delta_time);
	mup_scene_manager->UpdateMatrices();
	mup_graphics_system->Update(mup_scene_manager->GetCurrentScene(), p_delta_time);
}

void Framework::Draw(float p_delta_time, float p_total_time)
{
	mup_graphics_system->Draw(mup_scene_manager->GetCurrentScene(), p_delta_time);
}