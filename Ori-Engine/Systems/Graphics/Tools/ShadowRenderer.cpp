#include "ShadowRenderer.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

ShadowRenderer::ShadowRenderer(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context)
	: mp_device(pp_device), mp_context(pp_context)
{
	mup_shadow_vertex_shader = std::make_unique<VertexShader>(mp_device, mp_context);
	if (!mup_shadow_vertex_shader->InitializeShaderFromFile(L"x64/Debug/shadow_vertex.cso"))
		mup_shadow_vertex_shader->InitializeShaderFromFile(L"shadow_vertex.cso");

	//m_SHADOW_MAP_SIZE = 1024;
	D3D11_SAMPLER_DESC shadow_sampler_desc = {};
	shadow_sampler_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	shadow_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadow_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadow_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadow_sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	mp_device->CreateSamplerState(&shadow_sampler_desc, mcp_shadow_sampler.GetAddressOf());
	D3D11_RASTERIZER_DESC shadow_rasterizer_desc = {};
	shadow_rasterizer_desc.FillMode = D3D11_FILL_SOLID;							// try setting depth bias and clamp to 0
	shadow_rasterizer_desc.CullMode = D3D11_CULL_BACK;
	shadow_rasterizer_desc.DepthBias = 1000;									// Multiplied by (smallest possible value > 0 in depth buffer)		
	shadow_rasterizer_desc.DepthBiasClamp = 0.0f;								// CLAMPED TO 0??
	shadow_rasterizer_desc.SlopeScaledDepthBias = 1.0f;
	shadow_rasterizer_desc.DepthClipEnable = true;
	mp_device->CreateRasterizerState(&shadow_rasterizer_desc, mcp_shadow_rasterizer.GetAddressOf());
}


ShadowRenderer::~ShadowRenderer()
{
}

void ShadowRenderer::EnableShadowing(Entity& pr_light)
{
	TransformComponent& transform_component = pr_light.GetTransformComponent();
	LightComponent* light_component = pr_light.GetComponentByType<LightComponent>();
	light_component->mup_shadow = std::move(std::make_unique<Shadow>());
	Shadow& shadow = *light_component->mup_shadow.get();

	// View
	XMMATRIX shadow_view_matrix = XMMatrixLookToLH(
		XMVectorSet(transform_component.m_position.x, transform_component.m_position.y, transform_component.m_position.z, 0),	// Light position
		XMVectorSet(transform_component.m_rotation.x, transform_component.m_rotation.y, transform_component.m_rotation.z, 0),	// Light direction
		XMVectorSet(0, 0, 1, 0));																								// Up direction
	XMStoreFloat4x4(&shadow.shadowViewMatrix, XMMatrixTranspose(shadow_view_matrix));

	// Projection (type dependant)
	if (DirectionalLight* light = dynamic_cast<DirectionalLight*>(light_component->GetGenericLight()))
	{
		//std::shared_ptr<DirectionalLight> directionalLight = std::dynamic_pointer_cast<DirectionalLight>(light);
		XMMATRIX shadow_projection_matrix = XMMatrixOrthographicLH(
			10.f,					// View Width
			10.f,					// View Height
			0.1f,					// Near clip	
			20.0f);					// Far clip
		XMStoreFloat4x4(&shadow.shadowProjectionMatrix, XMMatrixTranspose(shadow_projection_matrix));
	}

	D3D11_TEXTURE2D_DESC shadow_texture_desc = {};
	shadow_texture_desc.Width = m_SHADOW_MAP_SIZE;
	shadow_texture_desc.Height = m_SHADOW_MAP_SIZE;
	shadow_texture_desc.ArraySize = 1;
	shadow_texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadow_texture_desc.CPUAccessFlags = 0;
	shadow_texture_desc.Format = DXGI_FORMAT_R16_TYPELESS;
	shadow_texture_desc.MipLevels = 1;
	shadow_texture_desc.MiscFlags = 0;
	shadow_texture_desc.SampleDesc.Count = 1;
	shadow_texture_desc.SampleDesc.Quality = 0;
	shadow_texture_desc.Usage = D3D11_USAGE_DEFAULT;
	ID3D11Texture2D* shadow_texture;
	mp_device->CreateTexture2D(&shadow_texture_desc, 0, &shadow_texture);
	std::string shadow_map_name("Shadow Map Texture");
	shadow_texture->SetPrivateData(WKPDID_D3DDebugObjectName, shadow_map_name.size(), shadow_map_name.c_str());

	D3D11_DEPTH_STENCIL_VIEW_DESC shadow_dsv_desc = {};
	shadow_dsv_desc.Format = DXGI_FORMAT_D16_UNORM;
	shadow_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadow_dsv_desc.Texture2D.MipSlice = 0;
	mp_device->CreateDepthStencilView(shadow_texture, &shadow_dsv_desc, shadow.shadowDepthStencilView.GetAddressOf());
	std::string shadow_map_dsv_name("Shadow Map DSV");
	shadow.shadowDepthStencilView.Get()->SetPrivateData(WKPDID_D3DDebugObjectName, shadow_map_dsv_name.size(), shadow_map_dsv_name.c_str());

	D3D11_SHADER_RESOURCE_VIEW_DESC shadow_srv_desc = {};
	shadow_srv_desc.Format = DXGI_FORMAT_R16_UNORM;
	shadow_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shadow_srv_desc.Texture2D.MipLevels = 1;
	shadow_srv_desc.Texture2D.MostDetailedMip = 0;
	mp_device->CreateShaderResourceView(shadow_texture, &shadow_srv_desc, shadow.shadowShaderResourceView.GetAddressOf());
	std::string shadow_map_srv_name("Shadow Map SRV");
	shadow.shadowShaderResourceView.Get()->SetPrivateData(WKPDID_D3DDebugObjectName, shadow_map_srv_name.size(), shadow_map_srv_name.c_str());

	shadow_texture->Release();
}

void ShadowRenderer::EnableCascadedShadowing(Entity& pr_light, UINT p_num_cascades)
{
	LightComponent* light_component = pr_light.GetComponentByType<LightComponent>();
	if (light_component->mup_shadow.get() == NULL)
	{
		light_component->mup_shadow = std::move(std::make_unique<Shadow>());
	}
	std::unique_ptr<Shadow>& shadow = light_component->mup_shadow;
	shadow = std::make_unique<CascadedShadow>(*light_component->mup_shadow.get());
	CascadedShadow& cascaded_shadow = static_cast<CascadedShadow&>(*shadow.get());
	cascaded_shadow.m_num_cascades = p_num_cascades;

	D3D11_TEXTURE2D_DESC cascaded_shadow_texture_desc = {};
	cascaded_shadow_texture_desc.Width				= m_SHADOW_MAP_SIZE;
	cascaded_shadow_texture_desc.Height				= m_SHADOW_MAP_SIZE;
	cascaded_shadow_texture_desc.ArraySize			= cascaded_shadow.m_num_cascades;
	cascaded_shadow_texture_desc.BindFlags			= D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	cascaded_shadow_texture_desc.CPUAccessFlags		= 0;
	cascaded_shadow_texture_desc.Format				= DXGI_FORMAT_R16_TYPELESS;
	cascaded_shadow_texture_desc.MipLevels			= 1;
	cascaded_shadow_texture_desc.MiscFlags			= 0;
	cascaded_shadow_texture_desc.SampleDesc.Count	= 1;
	cascaded_shadow_texture_desc.SampleDesc.Quality = 0;
	cascaded_shadow_texture_desc.Usage				= D3D11_USAGE_DEFAULT;
	mp_device->CreateTexture2D(&cascaded_shadow_texture_desc, 0, cascaded_shadow.mcp_cascade_texture.GetAddressOf());
	std::string csm_name("Cascaded Shadow Map Texture Array");
	cascaded_shadow.mcp_cascade_texture.Get()->SetPrivateData(WKPDID_D3DDebugObjectName, csm_name.size(), csm_name.c_str());
	D3D11_SHADER_RESOURCE_VIEW_DESC cascaded_shadow_srv_desc = {};
	cascaded_shadow_srv_desc.Format							= DXGI_FORMAT_R16_UNORM;
	cascaded_shadow_srv_desc.ViewDimension					= D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	cascaded_shadow_srv_desc.Texture2DArray.MostDetailedMip = 0;
	cascaded_shadow_srv_desc.Texture2DArray.MipLevels		= 1;
	cascaded_shadow_srv_desc.Texture2DArray.FirstArraySlice = 0;
	cascaded_shadow_srv_desc.Texture2DArray.ArraySize		= cascaded_shadow.m_num_cascades;
	mp_device->CreateShaderResourceView(cascaded_shadow.mcp_cascade_texture.Get(), &cascaded_shadow_srv_desc, cascaded_shadow.shadowShaderResourceView.GetAddressOf());
	std::string csm_srv_name("Cascaded Shadow Map SRV Array");
	cascaded_shadow.shadowShaderResourceView.Get()->SetPrivateData(WKPDID_D3DDebugObjectName, csm_srv_name.size(), csm_srv_name.c_str());

	for (UINT cascade_i = 0; cascade_i < cascaded_shadow.m_num_cascades; cascade_i++)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC cascade_dsv_desc = {};
		cascade_dsv_desc.Format							= DXGI_FORMAT_D16_UNORM;
		cascade_dsv_desc.ViewDimension					= D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		cascade_dsv_desc.Flags							= 0;
		cascade_dsv_desc.Texture2DArray.MipSlice		= 0;
		cascade_dsv_desc.Texture2DArray.FirstArraySlice = cascade_i;
		cascade_dsv_desc.Texture2DArray.ArraySize		= 1;

		cascaded_shadow.mcp_cascade_dsvs.emplace_back();
		mp_device->CreateDepthStencilView(cascaded_shadow.mcp_cascade_texture.Get(), &cascade_dsv_desc, cascaded_shadow.mcp_cascade_dsvs[cascade_i].GetAddressOf());
		std::string csm_dsv_name("Cascaded Shadow Map Cascade " + std::to_string(cascade_i) + " DSV");
		cascaded_shadow.mcp_cascade_dsvs[cascade_i].Get()->SetPrivateData(WKPDID_D3DDebugObjectName, csm_dsv_name.size(), csm_dsv_name.c_str());
	}
}

const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ShadowRenderer::GetSampler() const
{
	return mcp_shadow_sampler;
}

/*
// =====================================================================================================
//		Shadow Mapping
// =====================================================================================================
void ShadowRenderer::RenderShadowMap(Entity& reLight, std::vector<Entity&>& rvpeOpaques)
{
	Shadow* shadow = reLight->GetComponentByType<LightComponent>()->GetShadow();
	
	// Clear target
	mp_context->ClearDepthStencilView(shadow->shadowDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	
	// Set drawing states
	mp_context->OMSetRenderTargets(0, NULL, shadow->shadowDepthStencilView);
	mp_context->RSSetState(mcp_shadow_rasterizer.Get());
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (float)m_SHADOW_MAP_SIZE;
	vp.Height = (float)m_SHADOW_MAP_SIZE;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	mp_context->RSSetViewports(1, &vp);
	
	// Prepare Shaders
	mup_shadow_vertex_shader->SetShader();
	mp_context->PSSetShader(0, 0, 0);

	mup_shadow_vertex_shader->SetConstantBufferMatrix4x4("view", shadow->shadowViewMatrix);
	mup_shadow_vertex_shader->SetConstantBufferMatrix4x4("projection", shadow->shadowProjectionMatrix);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	for (Entity& entity : rvpeOpaques)
	{
		Mesh* mesh = entity.GetComponentByType<DrawComponent>()->GetMesh();
		mup_shadow_vertex_shader->SetConstantBufferMatrix4x4("world", entity.transform.GetWorldMatrix());
		mup_shadow_vertex_shader->UpdateAllConstantBuffers();

		// Draw every opaque into shadow map
		ID3D11Buffer* vb = mesh->GetVertexBuffer();
		ID3D11Buffer* ib = mesh->GetIndexBuffer();
		mp_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		mp_context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

		mp_context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
	}

	// Clear binds
	mp_context->OMSetRenderTargets(0, 0, 0);
	vp.Width = (float)*width;
	vp.Height = (float)*height;
	mp_context->RSSetViewports(1, &vp);
	mp_context->RSSetState(0);
	mp_context->PSSetShaderResources(0, 0, 0);
}
*/

// =====================================================================================================
//		Create an orthographic projection for each cascade
// =====================================================================================================
void ShadowRenderer::CalculateCascadeProjections(Entity& pr_light, const Entity& pr_camera)
{
	CascadedShadow& cascaded_shadow = static_cast<CascadedShadow&>(*pr_light.GetComponentByType<LightComponent>()->mup_shadow.get());

	float cascade_partitions[] = { 0.0f, 0.10f, 0.30f, .5f, 1.0f };		// percentage of camera frustrum z range
	CameraComponent* camera_component = pr_camera.GetComponentByType<CameraComponent>();
	float camera_frustum_range = camera_component->GetFarClip() - camera_component->GetNearClip();

	XMMATRIX camera_inverse_view_matrix = XMMatrixTranspose(XMLoadFloat4x4(&camera_component->m_inverse_view));
	XMMATRIX shadow_view_matrix = XMMatrixTranspose(XMLoadFloat4x4(&cascaded_shadow.shadowViewMatrix));
	float camera_aspect_ratio = camera_component->GetAspectRatio();
	float camera_fov = camera_component->GetFov();
	float tan_half_camera_fov_y = tanf(camera_fov / 2.0f);
	float tan_half_camera_fov_x = tanf((camera_fov * camera_aspect_ratio) / 2.0f);

	for (int cascade_i = 0; cascade_i < cascaded_shadow.m_num_cascades; cascade_i++)
	{
		// -----------------------------------------------------------------------------------------------------
		//		Assign this cascade to a partitioned interval of the camera's frustrum
		// -----------------------------------------------------------------------------------------------------
		float cascade_partition_near = cascade_partitions[cascade_i] * camera_frustum_range;
		float cascade_partition_far = cascade_partitions[cascade_i + 1] * camera_frustum_range;

		// -----------------------------------------------------------------------------------------------------
		//		Calculate this partition's frustum corners in cameraVS
		// -----------------------------------------------------------------------------------------------------
		float frustum_x_near = cascade_partition_near * tan_half_camera_fov_x;
		float frustum_x_far = cascade_partition_far * tan_half_camera_fov_x;
		float frustum_y_near = cascade_partition_near * tan_half_camera_fov_y;
		float frustum_y_far = cascade_partition_far * tan_half_camera_fov_y;
		XMFLOAT4 cascade_frustum_corners[8] = {
			{ frustum_x_near, frustum_y_near, cascade_partition_near, 1.0 },		// top right (near)
			{ -frustum_x_near, frustum_y_near, cascade_partition_near, 1.0 },		// top left
			{ frustum_x_near, -frustum_y_near, cascade_partition_near, 1.0 },		// bottom right
			{ -frustum_x_near, -frustum_y_near, cascade_partition_near, 1.0 },		// bottom left
			{ frustum_x_far, frustum_y_far, cascade_partition_far, 1.0 },			// top right (far)
			{ -frustum_x_far, frustum_y_far, cascade_partition_far, 1.0 },			// top left
			{ frustum_x_far, -frustum_y_far, cascade_partition_far, 1.0 },			// bottom right
			{ -frustum_x_far, -frustum_y_far, cascade_partition_far, 1.0 }			// bottom left
		};


		// -----------------------------------------------------------------------------------------------------
		//		Create a bounding box from the frustum in light space
		// -----------------------------------------------------------------------------------------------------
		float min_x = FLT_MAX, min_y = FLT_MAX, min_z = FLT_MAX;
		float max_x = FLT_MIN, max_y = FLT_MIN, max_z = FLT_MIN;
		for (int corner_i = 0; corner_i < 8; corner_i++)
		{
			// -----------------------------------------------------------------------------------------------------
			//		Transform these frustum's corners from view -> world -> light space
			// -----------------------------------------------------------------------------------------------------
			XMStoreFloat4(&cascade_frustum_corners[corner_i],
				XMVector4Transform(XMVector4Transform(XMLoadFloat4(&cascade_frustum_corners[corner_i]),
					camera_inverse_view_matrix),
					shadow_view_matrix));

			min_x = min(min_x, cascade_frustum_corners[corner_i].x);
			max_x = max(max_x, cascade_frustum_corners[corner_i].x);
			min_y = min(min_y, cascade_frustum_corners[corner_i].y);
			max_y = max(max_y, cascade_frustum_corners[corner_i].y);
			min_z = min(min_z, cascade_frustum_corners[corner_i].z);
			max_z = max(max_z, cascade_frustum_corners[corner_i].z);
		}
		// -----------------------------------------------------------------------------------------------------
		//		Create this cascade's orthographic projection
		// -----------------------------------------------------------------------------------------------------
		XMStoreFloat4x4(&cascaded_shadow.m_cascade_projections[cascade_i], XMMatrixTranspose(XMMatrixOrthographicOffCenterLH(
			min_x,			// ViewLeft
			max_x,			// ViewRight
			min_y,			// ViewBottom
			max_y,			// ViewTop
			min_z - 5,		// NearZ		// temporary fix to models not appearing in shadow map near transitions between cascades
			max_z)));		// FarZ

		if (cascade_i < cascaded_shadow.m_num_cascades - 1)
		{
			cascaded_shadow.m_cascade_far_clips[cascade_i] = cascade_partition_far;
		}
	}
}

/*	=====================================================================================================
		Cascaded Shadow Mapping

	=====================================================================================================	*/
void ShadowRenderer::RenderCascadeShadowMap(const std::vector<std::unique_ptr<Entity>>& pOpaqueEntities, Entity& pr_light, const Entity& pr_camera)
{
	CalculateCascadeProjections(pr_light, pr_camera);	// optimize to only call on camView || shadowView change?

	mup_shadow_vertex_shader->SetShader();
	mp_context->PSSetShader(0, 0, 0);

	// Match viewport to target texture resolution
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (float)m_SHADOW_MAP_SIZE;
	vp.Height = (float)m_SHADOW_MAP_SIZE;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	mp_context->RSSetViewports(1, &vp);
	mp_context->RSSetState(mcp_shadow_rasterizer.Get());

	CascadedShadow& cascaded_shadow = static_cast<CascadedShadow&>(*pr_light.GetComponentByType<LightComponent>()->mup_shadow.get());
	XMMATRIX shadow_view_matrix = XMMatrixTranspose(XMLoadFloat4x4(&cascaded_shadow.shadowViewMatrix));

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	for (int cascade_i = 0; cascade_i < cascaded_shadow.m_num_cascades; cascade_i++)
	{
		mp_context->ClearDepthStencilView(cascaded_shadow.mcp_cascade_dsvs[cascade_i].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		mp_context->OMSetRenderTargets(0, 0, cascaded_shadow.mcp_cascade_dsvs[cascade_i].Get());

		XMMATRIX shadow_cascade_projection_matrix = XMMatrixTranspose(XMLoadFloat4x4(&cascaded_shadow.m_cascade_projections[cascade_i]));

		// optimize to only those with bounding box overlap projection frustum pulled back towards light source (infinite for sun)
		for (int i = 0; i < pOpaqueEntities.size(); i++)
		{
			Entity* entity = pOpaqueEntities[i].get();

			if (!(entity->HasComponent<DrawComponent>())) { continue; }

			XMMATRIX entity_world_matrix = XMMatrixTranspose(XMLoadFloat4x4(&entity->GetTransformComponent().m_world_matrix));
			XMMATRIX temp_world_view_projection_matrix = entity_world_matrix * shadow_view_matrix * shadow_cascade_projection_matrix;
			XMFLOAT4X4 world_view_projection_matrix;
			XMStoreFloat4x4(&world_view_projection_matrix, XMMatrixTranspose(temp_world_view_projection_matrix));
			mup_shadow_vertex_shader->SetConstantBufferMatrix4x4("world_view_projection_matrix", world_view_projection_matrix);
			mup_shadow_vertex_shader->UpdateAllConstantBuffers();

			Mesh* mesh = entity->GetComponentByType<DrawComponent>()->GetMesh();
			ID3D11Buffer* vb = mesh->GetVertexBuffer();
			ID3D11Buffer* ib = mesh->GetIndexBuffer();
			mp_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
			mp_context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

			mp_context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
		}
	}

	// Clear binds
	mp_context->OMSetRenderTargets(0, 0, 0);
	mp_context->RSSetViewports(0, 0);
	mp_context->RSSetState(0);
}