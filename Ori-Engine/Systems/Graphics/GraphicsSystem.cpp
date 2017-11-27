#include "GraphicsSystem.h"

GraphicsSystem::GraphicsSystem(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, IDXGISwapChain* pp_swapchain,
	ID3D11RenderTargetView* pp_backbuffer_rtv, const unsigned int& pr_width, const unsigned int& pr_height)
	: mp_device(pp_device), mp_context(pp_context), mp_swapchain(pp_swapchain), mp_backbuffer_rtv(pp_backbuffer_rtv), 
	mr_width(pr_width), mr_height(pr_height)
{
	mp_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	//wireframe toggle?

	// Sampler
	D3D11_SAMPLER_DESC samplerdescAnsiWrap = {};	// toggle filtering method
	samplerdescAnsiWrap.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerdescAnsiWrap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdescAnsiWrap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdescAnsiWrap.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdescAnsiWrap.MaxAnisotropy = 16;
	samplerdescAnsiWrap.MaxLOD = D3D11_FLOAT32_MAX;
	mp_device->CreateSamplerState(&samplerdescAnsiWrap, mcp_sampler_filtering_choice.GetAddressOf());

	// Tools
	mup_shadow_renderer = std::make_unique<ShadowRenderer>(mp_device, mp_context);	// try making tools non pointers in initializer list
	mup_quad_renderer = std::make_unique<QuadRenderer>(mp_device, mp_context);
	mup_image_based_lighting_baker = std::make_unique<ImageBasedLightingBaker>(mp_device, mp_context, *mup_quad_renderer.get());
	mup_deferred_renderer = std::make_unique<DeferredRenderer>(mp_device, mp_context, mr_width, mr_height, mcp_sampler_filtering_choice.Get(), *mup_shadow_renderer.get(), *mup_quad_renderer.get());
	mup_skybox_renderer = std::make_unique<SkyBoxRenderer>(mp_device, mp_context);
	mup_post_processor = std::make_unique<PostProcessor>(mp_device, mp_context, mr_width, mr_height, *mup_quad_renderer.get());

	/*
	// =====================================================================================================
	//	Transparency Resources
	// =====================================================================================================
	// disable cull rasterizer
	D3D11_RASTERIZER_DESC rd = {};
	rd.CullMode = D3D11_CULL_NONE;
	rd.FillMode = D3D11_FILL_SOLID;
	rd.DepthClipEnable = true;
	mp_device->CreateRasterizerState(&rd, transparencyRasterizer.GetAddressOf());
	// blending amongst transparents
	D3D11_BLEND_DESC bd = {};
	bd.AlphaToCoverageEnable = false;
	bd.IndependentBlendEnable = false;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	mp_device->CreateBlendState(&bd, transparencyBlend.GetAddressOf());
	*/
}

GraphicsSystem::~GraphicsSystem()
{
}

ShadowRenderer * GraphicsSystem::GetShadowRenderer()
{
	return mup_shadow_renderer.get();
}

ImageBasedLightingBaker * GraphicsSystem::GetImageBasedLightingBaker()
{
	return mup_image_based_lighting_baker.get();
}

void GraphicsSystem::Draw(Scene* pp_current_scene, float p_delta_time)
{
	if (pp_current_scene == nullptr || pp_current_scene->GetCurrentCamera() == nullptr || pp_current_scene->GetCurrentSkyBox() == nullptr)
	{
		return;
	}

	mup_shadow_renderer->RenderCascadeShadowMap(pp_current_scene->GetEntities(), *pp_current_scene->mSun, *pp_current_scene->GetCurrentCamera());
	mup_deferred_renderer->PopulateGBuffers(pp_current_scene->GetEntities(), *pp_current_scene->GetCurrentCamera());
	// blur/soften shadow channel of gbuffer
	mup_deferred_renderer->CompositeShading(*pp_current_scene->GetCurrentCamera(), *pp_current_scene->mSun, *pp_current_scene->GetCurrentSkyBox(), mup_post_processor->GetFrameBufferRtv());
	mup_skybox_renderer->RenderStenciledSkyBox(pp_current_scene->GetCurrentSkyBox()->GetEnvMapSrv(), *pp_current_scene->GetCurrentCamera(), mup_deferred_renderer->GetDepthDsv(), mup_post_processor->GetFrameBufferRtv());
	// transparent draws
	mup_post_processor->GenerateAverageLuminance(mup_post_processor->GetFrameBufferSrv(), p_delta_time);
	mup_post_processor->Bloom(mup_post_processor->GetFrameBufferSrv());
	mup_post_processor->ToneMap(mup_post_processor->GetFrameBufferSrv(), mp_backbuffer_rtv);

	mp_swapchain->Present(0, 0);
}