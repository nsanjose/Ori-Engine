#include "DeferredRenderer.h"

#include <iostream>

using namespace DirectX;

DeferredRenderer::DeferredRenderer(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, const unsigned int& pr_Width, const unsigned int& pr_Height,
	const ShadowRenderer& pr_ShadowRenderer, QuadRenderer& pr_QuadRenderer)
	: mp_device(pp_device), mp_context(pp_context), mr_width(pr_Width), mr_height(pr_Height), m_shadow_renderer(pr_ShadowRenderer), mr_quad_renderer(pr_QuadRenderer)
{
	// Shaders
	mup_deferred_clear_gbuffer_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_deferred_clear_gbuffer_pixel_shader->InitializeShaderFromFile(L"x64/Debug/deferred_clear_gbuffer_pixel.cso"))
		mup_deferred_clear_gbuffer_pixel_shader->InitializeShaderFromFile(L"deferred_clear_gbuffer_pixel.cso");
	mup_deferred_buffer_vertex_shader = std::make_unique<VertexShader>(mp_device, mp_context);
	if (!mup_deferred_buffer_vertex_shader->InitializeShaderFromFile(L"x64/Debug/deferred_buffer_vertex.cso"))
		mup_deferred_buffer_vertex_shader->InitializeShaderFromFile(L"deferred_buffer_vertex.cso");
	mup_deferred_buffer_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_deferred_buffer_pixel_shader->InitializeShaderFromFile(L"x64/Debug/deferred_buffer_pixel.cso"))
		mup_deferred_buffer_pixel_shader->InitializeShaderFromFile(L"deferred_buffer_pixel.cso");
	mup_deferred_buffer_material_test_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_deferred_buffer_material_test_pixel_shader->InitializeShaderFromFile(L"x64/Debug/deferred_buffer_material_test_pixel.cso"))
		mup_deferred_buffer_material_test_pixel_shader->InitializeShaderFromFile(L"deferred_buffer_material_test_pixel.cso");
	mup_deferred_shadow_buffer_vertex_shader = std::make_unique<VertexShader>(mp_device, mp_context);
	if (!mup_deferred_shadow_buffer_vertex_shader->InitializeShaderFromFile(L"x64/Debug/deferred_shadow_buffer_vertex.cso"))
		mup_deferred_shadow_buffer_vertex_shader->InitializeShaderFromFile(L"deferred_shadow_buffer_vertex.cso");
	mup_deferred_shadow_buffer_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_deferred_shadow_buffer_pixel_shader->InitializeShaderFromFile(L"x64/Debug/deferred_shadow_buffer_pixel.cso"))
		mup_deferred_shadow_buffer_pixel_shader->InitializeShaderFromFile(L"deferred_shadow_buffer_pixel.cso");
	mup_deferred_composite_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_deferred_composite_pixel_shader->InitializeShaderFromFile(L"x64/Debug/deferred_composite_pixel.cso"))
		mup_deferred_composite_pixel_shader->InitializeShaderFromFile(L"deferred_composite_pixel.cso");
	mup_deferred_apply_ibl_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_deferred_apply_ibl_pixel_shader->InitializeShaderFromFile(L"x64/Debug/deferred_apply_ibl_pixel.cso"))
		mup_deferred_apply_ibl_pixel_shader->InitializeShaderFromFile(L"deferred_apply_ibl_pixel.cso");

	// Resources
	D3D11_SAMPLER_DESC sampler_point = {};
	ZeroMemory(&sampler_point, sizeof(D3D11_SAMPLER_DESC));
	sampler_point.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampler_point.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_point.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_point.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	mp_device->CreateSamplerState(&sampler_point, mcp_sampler_point.GetAddressOf());
	D3D11_SAMPLER_DESC sampler_bilinear_desc = {};
	ZeroMemory(&sampler_bilinear_desc, sizeof(D3D11_SAMPLER_DESC));
	sampler_bilinear_desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampler_bilinear_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_bilinear_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_bilinear_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	mp_device->CreateSamplerState(&sampler_bilinear_desc, mcp_sampler_bilinear.GetAddressOf());
	D3D11_SAMPLER_DESC sampler_ansio_wrap = {};
	sampler_ansio_wrap.Filter = D3D11_FILTER_ANISOTROPIC;
	sampler_ansio_wrap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_ansio_wrap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_ansio_wrap.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_ansio_wrap.MaxAnisotropy = 16;
	sampler_ansio_wrap.MaxLOD = D3D11_FLOAT32_MAX;
	mp_device->CreateSamplerState(&sampler_ansio_wrap, mcp_sampler_ansio.GetAddressOf());
	mp_sampler_filtering_choice = mcp_sampler_ansio.Get();

	D3D11_BLEND_DESC clear_shadow_buffer_desc = {};
	ZeroMemory(&clear_shadow_buffer_desc, sizeof(D3D11_BLEND_DESC));
	clear_shadow_buffer_desc.AlphaToCoverageEnable					= false;
	clear_shadow_buffer_desc.IndependentBlendEnable					= true;
	clear_shadow_buffer_desc.RenderTarget[0].BlendEnable			= true;
	clear_shadow_buffer_desc.RenderTarget[0].BlendOp				= D3D11_BLEND_OP_ADD;
	clear_shadow_buffer_desc.RenderTarget[0].SrcBlend				= D3D11_BLEND_ONE;
	clear_shadow_buffer_desc.RenderTarget[0].DestBlend				= D3D11_BLEND_ZERO;
	clear_shadow_buffer_desc.RenderTarget[0].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
	clear_shadow_buffer_desc.RenderTarget[0].SrcBlendAlpha			= D3D11_BLEND_ONE;
	clear_shadow_buffer_desc.RenderTarget[0].DestBlendAlpha			= D3D11_BLEND_ZERO;
	clear_shadow_buffer_desc.RenderTarget[0].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALPHA;
	clear_shadow_buffer_desc.RenderTarget[1].BlendEnable			= true;
	clear_shadow_buffer_desc.RenderTarget[1].BlendOp				= D3D11_BLEND_OP_ADD;
	clear_shadow_buffer_desc.RenderTarget[1].SrcBlend				= D3D11_BLEND_ZERO;
	clear_shadow_buffer_desc.RenderTarget[1].DestBlend				= D3D11_BLEND_ONE;
	clear_shadow_buffer_desc.RenderTarget[1].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
	clear_shadow_buffer_desc.RenderTarget[1].SrcBlendAlpha			= D3D11_BLEND_ZERO;
	clear_shadow_buffer_desc.RenderTarget[1].DestBlendAlpha			= D3D11_BLEND_ONE;
	clear_shadow_buffer_desc.RenderTarget[1].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
	clear_shadow_buffer_desc.RenderTarget[2].BlendEnable			= true;
	clear_shadow_buffer_desc.RenderTarget[2].BlendOp				= D3D11_BLEND_OP_ADD;
	clear_shadow_buffer_desc.RenderTarget[2].SrcBlend				= D3D11_BLEND_ZERO;
	clear_shadow_buffer_desc.RenderTarget[2].DestBlend				= D3D11_BLEND_ONE;
	clear_shadow_buffer_desc.RenderTarget[2].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
	clear_shadow_buffer_desc.RenderTarget[2].SrcBlendAlpha			= D3D11_BLEND_ZERO;
	clear_shadow_buffer_desc.RenderTarget[2].DestBlendAlpha			= D3D11_BLEND_ONE;
	clear_shadow_buffer_desc.RenderTarget[2].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
	mp_device->CreateBlendState(&clear_shadow_buffer_desc, mcp_clear_shadow_buffer_blend_state.GetAddressOf());

	D3D11_BLEND_DESC shadow_buffer_blend_desc = {};
	ZeroMemory(&shadow_buffer_blend_desc, sizeof(D3D11_BLEND_DESC));
	shadow_buffer_blend_desc.AlphaToCoverageEnable					= false;
	shadow_buffer_blend_desc.IndependentBlendEnable					= true;
	shadow_buffer_blend_desc.RenderTarget[0].BlendEnable			= true;
	shadow_buffer_blend_desc.RenderTarget[0].BlendOp				= D3D11_BLEND_OP_ADD;
	shadow_buffer_blend_desc.RenderTarget[0].SrcBlend				= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[0].DestBlend				= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[0].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
	shadow_buffer_blend_desc.RenderTarget[0].SrcBlendAlpha			= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[0].DestBlendAlpha			= D3D11_BLEND_ZERO;
	shadow_buffer_blend_desc.RenderTarget[0].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALPHA;
	shadow_buffer_blend_desc.RenderTarget[1].BlendEnable			= true;
	shadow_buffer_blend_desc.RenderTarget[1].BlendOp				= D3D11_BLEND_OP_ADD;
	shadow_buffer_blend_desc.RenderTarget[1].SrcBlend				= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[1].DestBlend				= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[1].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
	shadow_buffer_blend_desc.RenderTarget[1].SrcBlendAlpha			= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[1].DestBlendAlpha			= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[1].RenderTargetWriteMask	= 0;
	shadow_buffer_blend_desc.RenderTarget[2].BlendEnable			= true;
	shadow_buffer_blend_desc.RenderTarget[2].BlendOp				= D3D11_BLEND_OP_ADD;
	shadow_buffer_blend_desc.RenderTarget[2].SrcBlend				= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[2].DestBlend				= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[2].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
	shadow_buffer_blend_desc.RenderTarget[2].SrcBlendAlpha			= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[2].DestBlendAlpha			= D3D11_BLEND_ONE;
	shadow_buffer_blend_desc.RenderTarget[2].RenderTargetWriteMask	= 0;
	mp_device->CreateBlendState(&shadow_buffer_blend_desc, mcp_shadow_buffer_blend_state.GetAddressOf());

	D3D11_BLEND_DESC gbuffer_blend_desc = {};
	ZeroMemory(&gbuffer_blend_desc, sizeof(D3D11_BLEND_DESC));
	gbuffer_blend_desc.AlphaToCoverageEnable					= false;
	gbuffer_blend_desc.IndependentBlendEnable					= true;
	gbuffer_blend_desc.RenderTarget[0].BlendEnable				= true;
	gbuffer_blend_desc.RenderTarget[0].BlendOp					= D3D11_BLEND_OP_ADD;
	gbuffer_blend_desc.RenderTarget[0].SrcBlend					= D3D11_BLEND_ONE;
	gbuffer_blend_desc.RenderTarget[0].DestBlend				= D3D11_BLEND_ZERO;
	gbuffer_blend_desc.RenderTarget[0].BlendOpAlpha				= D3D11_BLEND_OP_ADD;
	gbuffer_blend_desc.RenderTarget[0].SrcBlendAlpha			= D3D11_BLEND_ZERO;
	gbuffer_blend_desc.RenderTarget[0].DestBlendAlpha			= D3D11_BLEND_ONE;
	gbuffer_blend_desc.RenderTarget[0].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
	gbuffer_blend_desc.RenderTarget[1].BlendEnable				= true;
	gbuffer_blend_desc.RenderTarget[1].BlendOp					= D3D11_BLEND_OP_ADD;
	gbuffer_blend_desc.RenderTarget[1].SrcBlend					= D3D11_BLEND_ONE;
	gbuffer_blend_desc.RenderTarget[1].DestBlend				= D3D11_BLEND_ZERO;
	gbuffer_blend_desc.RenderTarget[1].BlendOpAlpha				= D3D11_BLEND_OP_ADD;
	gbuffer_blend_desc.RenderTarget[1].SrcBlendAlpha			= D3D11_BLEND_ONE;
	gbuffer_blend_desc.RenderTarget[1].DestBlendAlpha			= D3D11_BLEND_ZERO;
	gbuffer_blend_desc.RenderTarget[1].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
	gbuffer_blend_desc.RenderTarget[2].BlendEnable				= true;
	gbuffer_blend_desc.RenderTarget[2].BlendOp					= D3D11_BLEND_OP_ADD;
	gbuffer_blend_desc.RenderTarget[2].SrcBlend					= D3D11_BLEND_ZERO;
	gbuffer_blend_desc.RenderTarget[2].DestBlend				= D3D11_BLEND_ONE;
	gbuffer_blend_desc.RenderTarget[2].BlendOpAlpha				= D3D11_BLEND_OP_ADD;
	gbuffer_blend_desc.RenderTarget[2].SrcBlendAlpha			= D3D11_BLEND_ONE;
	gbuffer_blend_desc.RenderTarget[2].DestBlendAlpha			= D3D11_BLEND_ZERO;
	gbuffer_blend_desc.RenderTarget[2].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALPHA;
	mp_device->CreateBlendState(&gbuffer_blend_desc, mcp_gbuffer_blend_state.GetAddressOf());

	D3D11_BLEND_DESC accumulate_shading_blend_desc = {};
	ZeroMemory(&accumulate_shading_blend_desc, sizeof(D3D11_BLEND_DESC));
	accumulate_shading_blend_desc.AlphaToCoverageEnable					= false;
	accumulate_shading_blend_desc.IndependentBlendEnable				= false;
	accumulate_shading_blend_desc.RenderTarget[0].BlendEnable			= true;
	accumulate_shading_blend_desc.RenderTarget[0].BlendOp				= D3D11_BLEND_OP_ADD;
	accumulate_shading_blend_desc.RenderTarget[0].SrcBlend				= D3D11_BLEND_ONE;
	accumulate_shading_blend_desc.RenderTarget[0].DestBlend				= D3D11_BLEND_ONE;
	accumulate_shading_blend_desc.RenderTarget[0].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
	accumulate_shading_blend_desc.RenderTarget[0].SrcBlendAlpha			= D3D11_BLEND_ZERO;
	accumulate_shading_blend_desc.RenderTarget[0].DestBlendAlpha		= D3D11_BLEND_ONE;
	accumulate_shading_blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
	mp_device->CreateBlendState(&accumulate_shading_blend_desc, mcp_accumulate_shading_blend_state.GetAddressOf());

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

void DeferredRenderer::ClearAllGBuffers()
{
	mp_context->ClearRenderTargetView(mcp_gbuffer_rtvs[0].Get(), m_gbuffer_0_clear_color);
	mp_context->ClearRenderTargetView(mcp_gbuffer_rtvs[1].Get(), m_gbuffer_1_clear_color);
	mp_context->ClearRenderTargetView(mcp_gbuffer_rtvs[2].Get(), m_gbuffer_2_clear_color);
	mp_context->ClearDepthStencilView(mcp_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DeferredRenderer::ClearShadowBuffer()
{
	mp_context->OMSetBlendState(mcp_clear_shadow_buffer_blend_state.Get(), 0, 0xfffffff);
	mr_quad_renderer.SetVertexShader();
	mup_deferred_clear_gbuffer_pixel_shader->SetShader();
	mp_context->OMSetRenderTargets(m_BUFFER_COUNT, mcp_gbuffer_rtvs[0].GetAddressOf(), 0);
	mr_quad_renderer.Draw();
	mp_context->OMSetRenderTargets(0, 0, 0);
}

void DeferredRenderer::PopulateShadowBuffer(const Entity& pr_light, const std::vector<std::unique_ptr<Entity>>& pp_occluders, const Entity& pr_camera)
{
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)mr_width, (float)mr_height, 0.0f, 1.0f };
	mp_context->RSSetViewports(1, &vp);
	
	// add variations for standard/cascaded

	mup_deferred_shadow_buffer_vertex_shader->SetShader();
	mup_deferred_shadow_buffer_pixel_shader->SetShader();
	mp_context->OMSetBlendState(mcp_shadow_buffer_blend_state.Get(), 0, 0xfffffff);
	mp_context->OMSetRenderTargets(m_BUFFER_COUNT, mcp_gbuffer_rtvs[0].GetAddressOf(), 0);

	// CSM
	mup_deferred_shadow_buffer_pixel_shader->SetSamplerState("shadow_sampler", m_shadow_renderer.GetSampler().Get());
	CascadedShadow& cascaded_shadow = *static_cast<CascadedShadow*>(pr_light.GetComponentByType<LightComponent>()->mup_shadow.get());
	mup_deferred_shadow_buffer_pixel_shader->SetShaderResourceView("cascaded_shadow_map", cascaded_shadow.shadowShaderResourceView.Get());
	mup_deferred_shadow_buffer_pixel_shader->SetConstantBufferMatrix4x4("shadow_view_matrix", cascaded_shadow.shadowViewMatrix);
	mup_deferred_shadow_buffer_pixel_shader->SetConstantBufferInt("num_cascades", cascaded_shadow.m_num_cascades);
	for (UINT i = 0; i < MAX_NUM_SHADOW_CASCADES - 1; i++)
	{
		// pack far clip into unused element of projection matrix to avoid extra transfer
		// unpack and remove from projection matrix in shader
		cascaded_shadow.m_cascade_projections[i]._41 = cascaded_shadow.m_cascade_far_clips[i];
	}
	mup_deferred_shadow_buffer_pixel_shader->SetConstantBufferVariable("cascade_projections", &cascaded_shadow.m_cascade_projections[0], sizeof(XMFLOAT4X4) * MAX_NUM_SHADOW_CASCADES);

	// PCF
	mup_deferred_shadow_buffer_pixel_shader->SetConstantBufferFloat("shadow_map_width", 1024);
	mup_deferred_shadow_buffer_pixel_shader->SetConstantBufferFloat("shadow_map_height", 1024);
	mup_deferred_shadow_buffer_pixel_shader->UpdateAllConstantBuffers();

	CameraComponent* camera_component = pr_camera.GetComponentByType<CameraComponent>();
	DirectX::XMMATRIX camera_view_matrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&camera_component->m_view_matrix));
	DirectX::XMMATRIX camera_projection_matrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&camera_component->m_projection_matrix));

	for (int i = 0; i < pp_occluders.size(); i++)
	{
		Entity* entity = pp_occluders[i].get();
		TransformComponent& transform_component = entity->GetTransformComponent();
		if (!(entity->HasComponent<DrawComponent>())) { continue; }
		Mesh* mesh = entity->GetComponentByType<DrawComponent>()->GetMesh();

		mup_deferred_shadow_buffer_vertex_shader->SetConstantBufferMatrix4x4("world_matrix", transform_component.m_world_matrix);
		DirectX::XMMATRIX entity_world_matrix		= DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&transform_component.m_world_matrix));
		DirectX::XMMATRIX temp_world_view_matrix	= entity_world_matrix * camera_view_matrix;
		DirectX::XMFLOAT4X4 world_view_matrix;
		DirectX::XMStoreFloat4x4(&world_view_matrix, XMMatrixTranspose(temp_world_view_matrix));
		mup_deferred_shadow_buffer_vertex_shader->SetConstantBufferMatrix4x4("world_view_matrix", world_view_matrix);
		DirectX::XMMATRIX temp_world_view_projection_matrix	= temp_world_view_matrix * camera_projection_matrix;
		DirectX::XMFLOAT4X4 world_view_projection_matrix;
		DirectX::XMStoreFloat4x4(&world_view_projection_matrix, XMMatrixTranspose(temp_world_view_projection_matrix));
		mup_deferred_shadow_buffer_vertex_shader->SetConstantBufferMatrix4x4("world_view_projection_matrix", world_view_projection_matrix);
		mup_deferred_shadow_buffer_vertex_shader->UpdateAllConstantBuffers();

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		ID3D11Buffer* vertex_buffer = mesh->GetVertexBuffer();
		ID3D11Buffer* indexBuffer	= mesh->GetIndexBuffer();
		mp_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
		mp_context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		mp_context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
	}

	// Unbind resources
	mup_deferred_buffer_pixel_shader->SetSamplerState("shadow_sampler", 0);
	mup_deferred_buffer_pixel_shader->SetShaderResourceView("cascaded_shadow_map", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);
	mp_context->OMSetBlendState(0, 0, 0xfffffff);
}

/*	=====================================================================================================
		Deferred Shading
		First Pass: Populate G-Buffer
	=====================================================================================================	*/
void DeferredRenderer::PopulateGBuffers(const std::vector<std::unique_ptr<Entity>>& pup_opaque_draw_entities, const Entity& pCameraEntity)
{
	mp_context->OMSetBlendState(mcp_gbuffer_blend_state.Get(), 0, 0xffffffff);

	//mp_context->ClearDepthStencilView(mcp_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

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

		mup_deferred_buffer_vertex_shader->SetConstantBufferMatrix4x4("world_matrix", transform_component.m_world_matrix);
		DirectX::XMMATRIX temp_entity_world_matrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&transform_component.m_world_matrix));
		DirectX::XMMATRIX temp_world_view_projection_matrix = temp_entity_world_matrix * temp_camera_view_matrix * temp_camera_projection_matrix;
		DirectX::XMFLOAT4X4 world_view_projection_matrix;
		DirectX::XMStoreFloat4x4(&world_view_projection_matrix, XMMatrixTranspose(temp_world_view_projection_matrix));
		mup_deferred_buffer_vertex_shader->SetConstantBufferMatrix4x4("world_view_projection_matrix", world_view_projection_matrix);

		if (material->IsTestMaterial())
		{
			mup_deferred_buffer_material_test_pixel_shader->SetShader();
			mup_deferred_buffer_material_test_pixel_shader->SetConstantBufferFloat3("base_color", material->GetBaseColor());
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
		ID3D11Buffer* indexBuffer	= mesh->GetIndexBuffer();
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
	mp_context->OMSetBlendState(0, 0, 0xfffffff);
}

/*	=====================================================================================================
		Deferred Shading
		Second Pass: Composite Lighting with G-Buffer
	=====================================================================================================	*/
void DeferredRenderer::CompositeShading(const Entity & pCamera, const Entity & pSun, ID3D11RenderTargetView * prtvFrameBuffer)
{
	CameraComponent* camera_component = pCamera.GetComponentByType<CameraComponent>();
	if (camera_component == nullptr) { return; }
	LightComponent* light_component = pSun.GetComponentByType<LightComponent>();
	if (light_component == nullptr) { return; }

	mp_context->OMSetBlendState(mcp_accumulate_shading_blend_state.Get(), 0, 0xfffffff);
	mr_quad_renderer.SetVertexShader();

	// dynamic shader linkage for light type
	Light* generic_light = light_component->GetGenericLight();
	if (DirectionalLight* directional_light = dynamic_cast<DirectionalLight*>(generic_light))
	{
		mup_deferred_composite_pixel_shader->SetInterfaceToClass("light", "directional_light");
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat3("directional_light:Light.color", directional_light->m_color);
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat("directional_light:Light.irradiance", directional_light->m_irradiance);
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat3("directional_light.direction", pSun.GetTransformComponent().m_rotation);
	}
	else if (SpotLight* spot_light = dynamic_cast<SpotLight*>(generic_light))
	{
		mup_deferred_composite_pixel_shader->SetInterfaceToClass("light", "spot_light");
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat3("spot_light:Light.color", spot_light->m_color);
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat("spot_light:Light.irradiance", spot_light->m_irradiance);
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat3("spot_light position_ws", pSun.GetTransformComponent().m_position);
	}
	else if (PointLight* point_light = dynamic_cast<PointLight*>(generic_light))
	{
		mup_deferred_composite_pixel_shader->SetInterfaceToClass("light", "point_light");
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat3("point_light:Light.color", point_light->m_color);
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat("point_light:Light.irradiance", point_light->m_irradiance);
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat3("point_light.position_ws", pSun.GetTransformComponent().m_position);
		mup_deferred_composite_pixel_shader->SetConstantBufferFloat("point_light.radius", point_light->m_radius);
	}
	mup_deferred_composite_pixel_shader->SetShader();

	// G-Buffer
	mup_deferred_composite_pixel_shader->SetSamplerState("sampler_point", mcp_sampler_point.Get());
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_0", mcp_gbuffer_srvs[0].Get());
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_1", mcp_gbuffer_srvs[1].Get());
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_2", mcp_gbuffer_srvs[2].Get());
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_3", mcp_depth_srv.Get());

	// Reconstructing positionWS
	mup_deferred_composite_pixel_shader->SetConstantBufferMatrix4x4("inverse_view_matrix", camera_component->m_inverse_view);
	mup_deferred_composite_pixel_shader->SetConstantBufferMatrix4x4("inverse_projection_matrix", camera_component->m_inverse_projection_matrix);
	mup_deferred_composite_pixel_shader->SetConstantBufferFloat3("camera_position_world_space", pCamera.GetTransformComponent().m_position);
	mup_deferred_composite_pixel_shader->SetConstantBufferFloat("camera_projection_a", camera_component->m_projection_a);
	mup_deferred_composite_pixel_shader->SetConstantBufferFloat("camera_projection_b", camera_component->m_projection_b);
	mup_deferred_composite_pixel_shader->UpdateAllConstantBuffers();

	mp_context->OMSetRenderTargets(1, &prtvFrameBuffer, 0);
	mr_quad_renderer.Draw((float)mr_width, (float)mr_height);

	// Unbind resources
	mup_deferred_composite_pixel_shader->SetSamplerState("sampler_point", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_0", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_1", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_2", 0);
	mup_deferred_composite_pixel_shader->SetShaderResourceView("gbuffer_3", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);
	mp_context->OMSetBlendState(0, 0, 0xfffffff);
}

void DeferredRenderer::ApplyIBL(const Entity& pr_camera, SkyBox* pp_skybox, ID3D11RenderTargetView* pp_destination_rtv)
{
	mp_context->OMSetBlendState(mcp_accumulate_shading_blend_state.Get(), 0, 0xfffffff);
	mr_quad_renderer.SetVertexShader();
	mup_deferred_apply_ibl_pixel_shader->SetShader();

	mup_deferred_apply_ibl_pixel_shader->SetSamplerState("sampler_point", mcp_sampler_point.Get());
	mup_deferred_apply_ibl_pixel_shader->SetSamplerState("sampler_ansio", mcp_sampler_ansio.Get());

	// G-Buffer
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("gbuffer_0", mcp_gbuffer_srvs[0].Get());
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("gbuffer_1", mcp_gbuffer_srvs[1].Get());
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("gbuffer_2", mcp_gbuffer_srvs[2].Get());
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("gbuffer_3", mcp_depth_srv.Get());
	// IBL
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("irradiance_map", pp_skybox->GetIrradianceMapSrv());
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("prefiltered_env_map", pp_skybox->GetPfemSrv());
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("brdf_lut", pp_skybox->GetBrdfLutSrv());

	// Reconstructing positionWS
	CameraComponent* camera_component = pr_camera.GetComponentByType<CameraComponent>();
	mup_deferred_apply_ibl_pixel_shader->SetConstantBufferMatrix4x4("inverse_view_matrix", camera_component->m_inverse_view);
	mup_deferred_apply_ibl_pixel_shader->SetConstantBufferMatrix4x4("inverse_projection_matrix", camera_component->m_inverse_projection_matrix);
	mup_deferred_apply_ibl_pixel_shader->SetConstantBufferFloat3("camera_position_world_space", pr_camera.GetTransformComponent().m_position);
	mup_deferred_apply_ibl_pixel_shader->SetConstantBufferFloat("camera_projection_a", camera_component->m_projection_a);
	mup_deferred_apply_ibl_pixel_shader->SetConstantBufferFloat("camera_projection_b", camera_component->m_projection_b);
	mup_deferred_apply_ibl_pixel_shader->UpdateAllConstantBuffers();

	mp_context->OMSetRenderTargets(1, &pp_destination_rtv, 0);
	mr_quad_renderer.Draw((float)mr_width, (float)mr_height);

	// Unbind resources
	mup_deferred_apply_ibl_pixel_shader->SetSamplerState("sampler_point", 0);
	mup_deferred_apply_ibl_pixel_shader->SetSamplerState("sampler_ansio", 0);
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("gbuffer_0", 0);
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("gbuffer_1", 0);
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("gbuffer_2", 0);
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("gbuffer_3", 0);
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("irradiance_map", 0);
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("prefiltered_env_map", 0);
	mup_deferred_apply_ibl_pixel_shader->SetShaderResourceView("brdf_lut", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);
	mp_context->OMSetBlendState(0, 0, 0xfffffff);
}

