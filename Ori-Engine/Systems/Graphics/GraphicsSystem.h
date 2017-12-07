#pragma once

#include <d3d11.h>
#include <vector>
#include <wrl.h>

#include "..\Scenes\SceneSystem.h"
#include "Tools\DeferredRenderer.h"
#include "Tools\ImageBasedLightingBaker.h"
#include "Tools\ShadowRenderer.h"
#include "Tools\SkyBoxRenderer.h"
#include "Tools\ParticleManager.h"
#include "Tools\PostProcessor.h"
#include "Tools\QuadRenderer.h"

class GraphicsSystem
{
public:
	GraphicsSystem(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, IDXGISwapChain* pp_swapchain,
		ID3D11RenderTargetView* pp_backbuffer_rtv, const unsigned int& pr_width, const unsigned int& pr_height);
	~GraphicsSystem();

	ShadowRenderer * GetShadowRenderer();
	ImageBasedLightingBaker * GetImageBasedLightingBaker();
	
	void Update(Scene* pp_current_scene, float p_delta_time);
	void Draw(Scene* pp_current_scene, float p_delta_time);

private:
	// References
	ID3D11Device* mp_device;
	ID3D11DeviceContext* mp_context;
	IDXGISwapChain* mp_swapchain;
	ID3D11RenderTargetView* mp_backbuffer_rtv;
	const unsigned int& mr_width;
	const unsigned int& mr_height;

	// Tools
	std::unique_ptr<DeferredRenderer> mup_deferred_renderer;
	std::unique_ptr<ImageBasedLightingBaker> mup_image_based_lighting_baker;
	std::unique_ptr<ShadowRenderer> mup_shadow_renderer;
	std::unique_ptr<SkyBoxRenderer> mup_skybox_renderer;
	std::unique_ptr<ParticleManager> mup_particle_manager;
	std::unique_ptr<PostProcessor> mup_post_processor;
	std::unique_ptr<QuadRenderer> mup_quad_renderer;

	// Resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_sampler_filtering_choice;

	/*
	// =====================================================================================================
	//	Transparency Resources
	// =====================================================================================================
	ComPtr<ID3D11RasterizerState> transparencyRasterizer;
	ComPtr<ID3D11BlendState> transparencyBlend;
	*/
};

