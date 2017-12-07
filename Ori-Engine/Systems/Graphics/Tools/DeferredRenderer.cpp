#include "DeferredRenderer.h"

#include <iostream>

DeferredRenderer::DeferredRenderer(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, const unsigned int& pr_Width, const unsigned int& pr_Height,
	ID3D11SamplerState* pp_CurrentSampler, const ShadowRenderer& pr_ShadowRenderer, QuadRenderer& pr_QuadRenderer)
	: mp_device(pp_device), mp_context(pp_context), mr_width(pr_Width), mr_height(pr_Height), mp_sampler_filtering_choice(pp_CurrentSampler),
	mShadowRenderer(pr_ShadowRenderer), mr_quad_renderer(pr_QuadRenderer)
{
	// Shaders
	mup_deferred_buffer_vertex_shader = std::make_unique<VertexShader>(mp_device, mp_context);
	if (!mup_deferred_buffer_vertex_shader->InitializeShaderFromFile(L"x64/Debug/deferred_buffer_vertex.cso"))
		mup_deferred_buffer_vertex_shader->InitializeShaderFromFile(L"deferred_buffer_vertex.cso");
	mup_deferred_buffer_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_deferred_buffer_pixel_shader->InitializeShaderFromFile(L"x64/Debug/deferred_buffer_pixel.cso"))
		mup_deferred_buffer_pixel_shader->InitializeShaderFromFile(L"deferred_buffer_pixel.cso");
	mup_deferred_composite_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_deferred_composite_pixel_shader->InitializeShaderFromFile(L"x64/Debug/deferred_composite_pixel.cso"))
		mup_deferred_composite_pixel_shader->InitializeShaderFromFile(L"deferred_composite_pixel.cso");

	mup_deferred_buffer_material_test_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_deferred_buffer_material_test_pixel_shader->InitializeShaderFromFile(L"x64/Debug/deferred_buffer_material_test_pixel.cso"))
		mup_deferred_buffer_material_test_pixel_shader->InitializeShaderFromFile(L"deferred_buffer_material_test_pixel.cso");

	// Resources
	D3D11_SAMPLER_DESC sampler_linear_desc = {};
	ZeroMemory(&sampler_linear_desc, sizeof(D3D11_SAMPLER_DESC));
	sampler_linear_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_linear_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_linear_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_linear_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	mp_device->CreateSamplerState(&sampler_linear_desc, mcp_sampler_linear.GetAddressOf());

	InitializeGBuffer();
}

DeferredRenderer::~DeferredRenderer()
{
}

ID3D11DepthStencilView* DeferredRenderer::GetDepthDsv()
{
	return mcp_depth_stencil_view.Get();
}

void DeferredRenderer::InitializeGBuffer()
{
	for (int i = 0; i < m_BUFFER_COUNT; i++)
	{
		D3D11_TEXTURE2D_DESC texture_desc		= {};
		texture_desc.Width						= mr_width;
		texture_desc.Height						= mr_height;
		texture_desc.MipLevels					= 1;
		texture_desc.ArraySize					= 1;
		texture_desc.Format						= DXGI_FORMAT_R16G16B16A16_FLOAT;
		texture_desc.SampleDesc.Count			= 1;
		texture_desc.SampleDesc.Quality			= 0;
		texture_desc.Usage						= D3D11_USAGE_DEFAULT;
		texture_desc.BindFlags					= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texture_desc.CPUAccessFlags				= 0;
		texture_desc.MiscFlags					= 0;
		mp_device->CreateTexture2D(&texture_desc, nullptr, mcp_gbuffer_textures[i].GetAddressOf());
		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc	= {};
		rtv_desc.Format							= texture_desc.Format;
		rtv_desc.ViewDimension					= D3D11_RTV_DIMENSION_TEXTURE2D;
		rtv_desc.Texture2D.MipSlice				= 0;
		mp_device->CreateRenderTargetView(mcp_gbuffer_textures[i].Get(), &rtv_desc, mcp_gbuffer_rtvs[i].GetAddressOf());
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc	= {};
		srv_desc.Format								= texture_desc.Format;
		srv_desc.ViewDimension						= D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MostDetailedMip			= 0;
		srv_desc.Texture2D.MipLevels				= 1;
		mp_device->CreateShaderResourceView(mcp_gbuffer_textures[i].Get(), &srv_desc, mcp_gbuffer_srvs[i].GetAddressOf());
	}
	
	// Depth/Stencil
	D3D11_TEXTURE2D_DESC depth_texture_desc	= {};
	depth_texture_desc.Width				= mr_width;
	depth_texture_desc.Height				= mr_height;
	depth_texture_desc.MipLevels			= 1;
	depth_texture_desc.ArraySize			= 1;
	depth_texture_desc.Format				= DXGI_FORMAT_R24G8_TYPELESS;
	depth_texture_desc.SampleDesc.Count		= 1;
	depth_texture_desc.SampleDesc.Quality	= 0;
	depth_texture_desc.Usage				= D3D11_USAGE_DEFAULT;
	depth_texture_desc.BindFlags			= D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depth_texture_desc.CPUAccessFlags		= 0;
	depth_texture_desc.MiscFlags			= 0;
	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc	= {};
	dsv_desc.Format							= DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsv_desc.ViewDimension					= D3D11_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Flags							= 0;
	dsv_desc.Texture2D.MipSlice				= 0;
	D3D11_SHADER_RESOURCE_VIEW_DESC depth_srv_desc	= {};
	depth_srv_desc.Format							= DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depth_srv_desc.ViewDimension					= D3D11_SRV_DIMENSION_TEXTURE2D;
	depth_srv_desc.Texture2D.MostDetailedMip		= 0;
	depth_srv_desc.Texture2D.MipLevels				= 1;
	ID3D11Texture2D* depth_texture;
	mp_device->CreateTexture2D(&depth_texture_desc, 0, &depth_texture);
	mp_device->CreateDepthStencilView(depth_texture, &dsv_desc, mcp_depth_stencil_view.GetAddressOf());
	mp_device->CreateShaderResourceView(depth_texture, &depth_srv_desc, mcp_depth_srv.GetAddressOf());
	depth_texture->Release();
}

/*	=====================================================================================================
		Deferred Shading
		First Pass: Populate G-Buffer

		Improve: Premultiply the WVP matrix sent to shader
	=====================================================================================================	*/
void DeferredRenderer::PopulateGBuffers(const std::vector<std::unique_ptr<Entity>>& pup_opaque_draw_entities, const Entity& pCameraEntity)
{
	for (int buffer_i = 0; buffer_i < m_BUFFER_COUNT; buffer_i++)
	{
		mp_context->ClearRenderTargetView(mcp_gbuffer_rtvs[buffer_i].Get(), m_clear_color);
	}
	mp_context->ClearDepthStencilView(mcp_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	mup_deferred_buffer_vertex_shader->SetShader();
	//mup_deferred_buffer_pixel_shader->SetShader();

	CameraComponent* camera_component = pCameraEntity.GetComponentByType<CameraComponent>();
	DirectX::XMMATRIX temp_camera_view_matrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&camera_component->m_view_matrix));
	DirectX::XMMATRIX temp_camera_projection_matrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&camera_component->m_projection_matrix));
	//mup_deferred_buffer_pixel_shader->SetSamplerState("sampler_filtering_choice", mp_sampler_filtering_choice);

	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)mr_width, (float)mr_height, 0.0f, 1.0f };
	mp_context->RSSetViewports(1, &vp);

	for (int i = 0; i < pup_opaque_draw_entities.size(); i++)
	{
		Entity* entity = pup_opaque_draw_entities[i].get();
		TransformComponent& transform_component = entity->GetTransformComponent();

		if (!(entity->HasComponent<DrawComponent>())) { continue; }

		DrawComponent* draw_component = entity->GetComponentByType<DrawComponent>();
		Mesh* mesh = draw_component->GetMesh();
		Material* material = draw_component->GetMaterial();

		/*
		DirectX::XMMATRIX temp_inverse_transpose_world_matrix = DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&entity->transform.GetWorldMatrix()));
		DirectX::XMFLOAT4X4 inverse_transpose_world_matrix;
		DirectX::XMStoreFloat4x4(&inverse_transpose_world_matrix, temp_inverse_transpose_world_matrix);
		mup_deferred_buffer_vertex_shader->SetConstantBufferMatrix4x4("inverse_transpose_world_matrix", entity->transform.GetWorldMatrix());
		*/
		mup_deferred_buffer_vertex_shader->SetConstantBufferMatrix4x4("world_matrix", transform_component.GetWorldMatrix());
		DirectX::XMMATRIX temp_entity_world_matrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&transform_component.GetWorldMatrix()));
		DirectX::XMMATRIX temp_world_view_projection_matrix = temp_entity_world_matrix * temp_camera_view_matrix * temp_camera_projection_matrix;
		DirectX::XMFLOAT4X4 world_view_projection_matrix;
		DirectX::XMStoreFloat4x4(&world_view_projection_matrix, XMMatrixTranspose(temp_world_view_projection_matrix));
		mup_deferred_buffer_vertex_shader->SetConstantBufferMatrix4x4("world_view_projection_matrix", world_view_projection_matrix);

		if (material->IsTestMaterial())
		{
			mup_deferred_buffer_material_test_pixel_shader->SetShader();
			mup_deferred_buffer_material_test_pixel_shader->SetConstantBufferFloat("metalness", material->GetTestMetalness());
			mup_deferred_buffer_material_test_pixel_shader->SetConstantBufferFloat("roughness", material->GetTestRoughness());
			mup_deferred_buffer_material_test_pixel_shader->UpdateAllConstantBuffers();
		}
		else
		{
			mup_deferred_buffer_pixel_shader->SetShader();
			mup_deferred_buffer_pixel_shader->SetSamplerState("sampler_filtering_choice", mp_sampler_filtering_choice);
			mup_deferred_buffer_pixel_shader->SetShaderResourceView("base_color_map", material->GetBaseColorMap());
			mup_deferred_buffer_pixel_shader->SetShaderResourceView("metalness_map", material->GetMetalnessMap());
			mup_deferred_buffer_pixel_shader->SetShaderResourceView("roughness_map", material->GetRoughnessMap());
			mup_deferred_buffer_pixel_shader->SetShaderResourceView("normal_map", material->GetNormalMap());
			mup_deferred_buffer_pixel_shader->SetShaderResourceView("ambient_occlusion_map", material->GetAmbientOcclusionMap());
			mup_deferred_buffer_pixel_shader->UpdateAllConstantBuffers();
		}

		mup_deferred_buffer_vertex_shader->UpdateAllConstantBuffers();
		//mup_deferred_buffer_pixel_shader->UpdateAllConstantBuffers();

		mp_context->OMSetRenderTargets(m_BUFFER_COUNT, mcp_gbuffer_rtvs[0].GetAddressOf(), mcp_depth_stencil_view.Get());

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		ID3D11Buffer* vertex_buffer = mesh->GetVertexBuffer();
		ID3D11Buffer* indexBuffer = mesh->GetIndexBuffer();
		mp_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
		mp_context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		mp_context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
	}
	
	// Unbind resources
	mup_deferred_buffer_pixel_shader->SetSamplerState("sampler_filtering_choice", 0);
	mup_deferred_buffer_pixel_shader->SetShaderResourceView("base_color_map", 0);
	mup_deferred_buffer_pixel_shader->SetShaderResourceView("metalness_map", 0);
	mup_deferred_buffer_pixel_shader->SetShaderResourceView("roughness_map", 0);
	mup_deferred_buffer_pixel_shader->SetShaderResourceView("normal_map", 0);
	mup_deferred_buffer_pixel_shader->SetShaderResourceView("ambient_occlusion_map", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);
}

void SendLightToShader(PixelShader* pixelShader, const Entity& pLight)
{
	LightComponent* light_component = pLight.GetComponentByType<LightComponent>();
	Light* light = light_component->GetGenericLight();
	if (typeid(*light) == typeid(DirectionalLight))
	{
		DirectionalLight* directionalLight = dynamic_cast<DirectionalLight*>(light);
		DirLightExport dirLightExport;
		dirLightExport.ambientColor = directionalLight->ambientColor;
		dirLightExport.diffuseColor = directionalLight->diffuseColor;
		dirLightExport.direction = pLight.GetTransformComponent().rotation;

		pixelShader->SetConstantBufferVariable("dL1", &dirLightExport, sizeof(DirLightExport));
	}
}

/*	=====================================================================================================
		Deferred Shading
		Second Pass: Composite Lighting with G-Buffer

		Improve: Move shadowing to G-Buffer pass/resource so it can be blurred/softened before lighting

		BROKEN: change pSun to vector of light entities, loop and draw for each. stencil the light volumes for optimization.
	=====================================================================================================	*/
void DeferredRenderer::CompositeShading(const Entity & pCamera, const Entity & pSun, SkyBox & pSkyBox, ID3D11RenderTargetView * prtvFrameBuffer)
{
	mr_quad_renderer.SetVertexShader();
	mup_deferred_composite_pixel_shader->SetShader();

	mup_deferred_composite_pixel_shader->SetSamplerState("sampler_linear", mp_sampler_filtering_choice);
	mup_deferred_composite_pixel_shader->SetSamplerState("sampler_ansio", mp_sampler_filtering_choice);
	// G-Buffer
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_0", mcp_gbuffer_srvs[0].Get());
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_1", mcp_gbuffer_srvs[1].Get());
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_2", mcp_depth_srv.Get());
	// Reconstructing positionWS
	CameraComponent* camera_component = pCamera.GetComponentByType<CameraComponent>();
	mup_deferred_composite_pixel_shader->SetConstantBufferMatrix4x4("inverse_view_matrix", camera_component->m_inverse_view);	// pre-multiply perspective divide
	mup_deferred_composite_pixel_shader->SetConstantBufferMatrix4x4("inverse_projection_matrix", camera_component->m_inverse_projection_matrix);
	mup_deferred_composite_pixel_shader->SetConstantBufferFloat3("camera_position_world_space", pCamera.GetTransformComponent().position);
	mup_deferred_composite_pixel_shader->SetConstantBufferFloat("camera_projection_a", camera_component->m_projection_a);
	mup_deferred_composite_pixel_shader->SetConstantBufferFloat("camera_projection_b", camera_component->m_projection_b);
	// IBL
	mup_deferred_composite_pixel_shader->SetShaderResourceView("irradiance_map", pSkyBox.GetIrradianceMapSrv());
	mup_deferred_composite_pixel_shader->SetShaderResourceView("prefiltered_env_map", pSkyBox.GetPfemSrv());
	mup_deferred_composite_pixel_shader->SetShaderResourceView("brdf_lut", pSkyBox.GetBrdfLutSrv());
	// CSM
	mup_deferred_composite_pixel_shader->SetSamplerState("shadow_sampler", mShadowRenderer.GetSampler().Get());
	CascadedShadow& cascaded_shadow = static_cast<CascadedShadow&>(*pSun.GetComponentByType<LightComponent>()->GetShadow());
	mup_deferred_composite_pixel_shader->SetShaderResourceView("sun_cascaded_shadow_maps", cascaded_shadow.shadowShaderResourceView.Get());
	mup_deferred_composite_pixel_shader->SetConstantBufferMatrix4x4("sun_view_matrix", cascaded_shadow.shadowViewMatrix);
	mup_deferred_composite_pixel_shader->SetConstantBufferFloat("sunCascade0EndDepth", cascaded_shadow.mCascadePartitionDepths[0]);	// place in empty (0) slot of related projection matrix, copy out and clear in shader
	mup_deferred_composite_pixel_shader->SetConstantBufferFloat("sunCascade1EndDepth", cascaded_shadow.mCascadePartitionDepths[1]);
	mup_deferred_composite_pixel_shader->SetConstantBufferVariable("sun_cascade_projections", &cascaded_shadow.mCascadeProjections[0], sizeof(cascaded_shadow.mCascadeProjections));

	// move shadowing to gbuffer pass

	// for each light
		// stencil volume
		// update cbuffer data for specific light
		// draw in either fullscreen quad or quad bounding box around volume in clip space
	SendLightToShader(mup_deferred_composite_pixel_shader.get(), pSun);

	mup_deferred_composite_pixel_shader->UpdateAllConstantBuffers();

	mp_context->ClearRenderTargetView(prtvFrameBuffer, m_clear_color);
	mp_context->OMSetRenderTargets(1, &prtvFrameBuffer, 0);
	mr_quad_renderer.Draw((float)mr_width, (float)mr_height);

	// Unbind resources
	mup_deferred_composite_pixel_shader->SetSamplerState("sampler_linear", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_0", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_1", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_2", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("irradiance_map", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("prefiltered_env_map", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("brdf_lut", 0);
	mup_deferred_composite_pixel_shader->SetSamplerState("shadow_sampler", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("sun_cascaded_shadow_maps", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);
}

