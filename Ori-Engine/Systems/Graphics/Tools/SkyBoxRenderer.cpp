#include "SkyBoxRenderer.h"

SkyBoxRenderer::SkyBoxRenderer(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context)
	: mp_device(pp_device), mp_context(pp_context)
{
	// Initialize Shaders
	mup_skybox_vertex_shader = std::make_unique<VertexShader>(mp_device, mp_context);
	if (!mup_skybox_vertex_shader->InitializeShaderFromFile(L"x64/Debug/skybox_vertex.cso"))
		mup_skybox_vertex_shader->InitializeShaderFromFile(L"skybox_vertex.cso");
	mup_skybox_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_skybox_pixel_shader->InitializeShaderFromFile(L"x64/Debug/skybox_pixel.cso"))
		mup_skybox_pixel_shader->InitializeShaderFromFile(L"skybox_pixel.cso");

	// Initialize Resources
	mup_cube_mesh = std::make_unique<Mesh>(mp_device, "Resources/Mesh Files/cube.obj");
	D3D11_RASTERIZER_DESC skybox_rasterizer_desc = {};
	skybox_rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	skybox_rasterizer_desc.CullMode = D3D11_CULL_FRONT;
	skybox_rasterizer_desc.DepthClipEnable = true;
	mp_device->CreateRasterizerState(&skybox_rasterizer_desc, mcp_skybox_rasterizer.GetAddressOf());
	D3D11_DEPTH_STENCIL_DESC skybox_depth_stencil_state = {};
	skybox_depth_stencil_state.DepthEnable = true;
	skybox_depth_stencil_state.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	skybox_depth_stencil_state.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	mp_device->CreateDepthStencilState(&skybox_depth_stencil_state, mcp_skybox_depth_stencil_state.GetAddressOf());
	D3D11_SAMPLER_DESC sampler_desc = {};
	ZeroMemory(&sampler_desc, sizeof(D3D11_SAMPLER_DESC));
	sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_desc.MaxAnisotropy = 16;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
	mp_device->CreateSamplerState(&sampler_desc, mcp_skybox_sampler.GetAddressOf());
}


SkyBoxRenderer::~SkyBoxRenderer()
{
}


void SkyBoxRenderer::RenderStenciledSkyBox(ID3D11ShaderResourceView* pp_skybox_srv, const Entity& pr_camera, ID3D11DepthStencilView* pp_depth_stencil_view,
	ID3D11RenderTargetView* pp_destination_rtv)
{
	mup_skybox_vertex_shader->SetShader();
	CameraComponent* camera_component = pr_camera.GetComponentByType<CameraComponent>();
	mup_skybox_vertex_shader->SetConstantBufferMatrix4x4("view_matrix", camera_component->m_view_matrix);
	mup_skybox_vertex_shader->SetConstantBufferMatrix4x4("projection_matrix", camera_component->m_projection_matrix);
	mup_skybox_vertex_shader->UpdateAllConstantBuffers();

	mup_skybox_pixel_shader->SetShader();
	mup_skybox_pixel_shader->SetSamplerState("sampler_ansio", mcp_skybox_sampler.Get());
	mup_skybox_pixel_shader->SetShaderResourceView("skybox", pp_skybox_srv);
	mup_skybox_pixel_shader->UpdateAllConstantBuffers();

	mp_context->RSSetState(mcp_skybox_rasterizer.Get());
	mp_context->OMSetRenderTargets(1, &pp_destination_rtv, pp_depth_stencil_view);
	mp_context->OMSetDepthStencilState(mcp_skybox_depth_stencil_state.Get(), 0);

	ID3D11Buffer* vertex_buffer = mup_cube_mesh->GetVertexBuffer();
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	mp_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
	mp_context->IASetIndexBuffer(mup_cube_mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mp_context->DrawIndexed(mup_cube_mesh->GetIndexCount(), 0, 0);

	// Unbind resources
	mup_skybox_pixel_shader->SetSamplerState("sampler_ansio", 0);
	mup_skybox_pixel_shader->SetShaderResourceView("skybox", 0);
	mp_context->RSSetState(0);
	mp_context->OMSetRenderTargets(0, 0, 0);
	mp_context->OMSetDepthStencilState(0, 0);
}