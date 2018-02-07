#include "GraphicsSystem.h"

GraphicsSystem::GraphicsSystem(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, IDXGISwapChain* pp_swapchain,
	ID3D11RenderTargetView* pp_backbuffer_rtv, const unsigned int& pr_width, const unsigned int& pr_height)
	: mp_device(pp_device), mp_context(pp_context), mp_swapchain(pp_swapchain), mp_backbuffer_rtv(pp_backbuffer_rtv), 
	mr_width(pr_width), mr_height(pr_height)
{
	mp_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	//wireframe toggle?

	// Tools
	mup_shadow_renderer = std::make_unique<ShadowRenderer>(mp_device, mp_context);	// try making tools non pointers in initializer list
	mup_quad_renderer = std::make_unique<QuadRenderer>(mp_device, mp_context);
	mup_image_based_lighting_baker = std::make_unique<ImageBasedLightingBaker>(mp_device, mp_context, *mup_quad_renderer.get());
	mup_deferred_renderer = std::make_unique<DeferredRenderer>(mp_device, mp_context, mr_width, mr_height, *mup_shadow_renderer.get(), *mup_quad_renderer.get());
	mup_skybox_renderer = std::make_unique<SkyBoxRenderer>(mp_device, mp_context);
	mup_particle_manager = std::make_unique<ParticleManager>(mp_device, mp_context);
	mup_post_processor = std::make_unique<PostProcessor>(mp_device, mp_context, mr_width, mr_height, *mup_quad_renderer.get());

	D3D11_QUERY_DESC are_any_pixels_lit_desc;
	are_any_pixels_lit_desc.Query		= D3D11_QUERY_OCCLUSION_PREDICATE;
	are_any_pixels_lit_desc.MiscFlags	= 0;
	mp_device->CreateQuery(&are_any_pixels_lit_desc, are_any_pixels_lit.GetAddressOf());

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

void GraphicsSystem::Update(Scene* pp_current_scene, float p_delta_time)
{
	mup_particle_manager->Update(mup_particle_manager->CollectEntitiesWithEmitters(pp_current_scene->GetEntities()), p_delta_time);
}

void GraphicsSystem::Draw(Scene* pp_current_scene, float p_delta_time)
{
	if (pp_current_scene == nullptr || pp_current_scene->GetCurrentCamera() == nullptr) { return; }

	ID3D11RenderTargetView* rtv = mp_backbuffer_rtv;
	if (pp_current_scene->is_post_processed) { rtv = mup_post_processor->GetFrameBufferRtv(); }

	// Start fresh
	float zero_color[4] = { 0, 0, 0, 0 };
	mp_context->ClearRenderTargetView(mp_backbuffer_rtv, zero_color);
	mp_context->ClearRenderTargetView(mup_post_processor->GetFrameBufferRtv(), zero_color);
	mup_deferred_renderer->ClearAllGBuffers();

	// Buffer material and spatial data
	mup_deferred_renderer->PopulateGBuffers(pp_current_scene->GetEntities(), *pp_current_scene->GetCurrentCamera());

	// Apply environmental lighting
	if (pp_current_scene->GetCurrentSkyBox() != nullptr)
	{
		mup_deferred_renderer->ApplyIBL(*pp_current_scene->GetCurrentCamera(), pp_current_scene->GetCurrentSkyBox(), rtv);
	}

	// Apply source lighting and shadowing
	for (int light_i = 0; light_i < pp_current_scene->GetLights().size(); light_i++)
	{
		//mp_context->Begin(are_any_pixels_lit.Get());
		// determine lit pixels	https://www.guerrilla-games.com/read/deferred-rendering-in-killzone-2
		// volume stencil
		//mp_context->End(are_any_pixels_lit.Get());
		//if (mp_context->GetData(are_any_pixels_lit.Get(), NULL, 0, 0))
		{
			// Reset shadow buffer for every light
			mup_deferred_renderer->ClearShadowBuffer();

			// Prepare shadow resources
			Shadow* shadow = pp_current_scene->GetLights()[light_i].get()->GetComponentByType<LightComponent>()->mup_shadow.get();
			if (shadow != nullptr)
			{
				if (CascadedShadow* cascaded_shadow = dynamic_cast<CascadedShadow*>(shadow))
				{
					mup_shadow_renderer->RenderCascadeShadowMap(pp_current_scene->GetEntities(), *pp_current_scene->GetLights()[light_i].get(), *pp_current_scene->GetCurrentCamera());
					// look into shimmering fix, remove sub-pixel movement https://www.guerrilla-games.com/read/the-rendering-technology-of-killzone-2
				}
				else
				{
					// no cascades, single map
				}
				// variation for point light with 6 single/cascaded maps?
				
				mup_deferred_renderer->PopulateShadowBuffer(*pp_current_scene->GetLights()[light_i].get(), pp_current_scene->GetEntities(), *pp_current_scene->GetCurrentCamera());
				// have a variation for sun that also samples pre-baked shadow map(s)?
				// additional soft shadowing? http://developer.download.nvidia.com/presentations/2008/GDC/GDC08_SoftShadowMapping.pdf
			}

			// Combine gbuffers with a light and its shadow
			mup_deferred_renderer->CompositeShading(*pp_current_scene->GetCurrentCamera(), *pp_current_scene->GetLights()[light_i].get(), rtv);
		}
	}

	// Slap particles on top
	mup_particle_manager->Render(*pp_current_scene->GetCurrentCamera()->GetComponentByType<CameraComponent>(), mup_particle_manager->CollectEntitiesWithEmitters(pp_current_scene->GetEntities()), rtv, mup_deferred_renderer->GetDepthDsv());
	
	// Fill everything else with sky
	if (pp_current_scene->GetCurrentSkyBox() != nullptr)
	{
		mup_skybox_renderer->RenderStenciledSkyBox(pp_current_scene->GetCurrentSkyBox()->GetEnvMapSrv(), *pp_current_scene->GetCurrentCamera(), mup_deferred_renderer->GetDepthDsv(), rtv);
	}

	// Transparent draws

	// Make it pretty
	if (pp_current_scene->is_post_processed)
	{
		mup_post_processor->GenerateAverageLuminance(mup_post_processor->GetFrameBufferSrv(), p_delta_time);
		mup_post_processor->Bloom(mup_post_processor->GetFrameBufferSrv());
		mup_post_processor->ToneMap(mup_post_processor->GetFrameBufferSrv(), mp_backbuffer_rtv);
	}

	mp_swapchain->Present(0, 0);
}