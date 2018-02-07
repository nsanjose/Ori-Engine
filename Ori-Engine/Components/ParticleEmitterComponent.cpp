#include "ParticleEmitterComponent.h"

#include <WICTextureLoader.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

ParticleEmitterComponent::ParticleEmitterComponent(TransformComponent& pr_transform, ID3D11Device* pp_device) : Component(),
	m_time_since_emission		(0.0f),

	// Settings
	m_is_emitting				(true),
	m_emissions_per_second		(200.f),
	m_max_particle_age			(.5f),
	m_max_particle_count		(500),
	m_particle_half_width		(0.01f),
	m_particle_half_height		(0.1f),

	m_particle_position_x_variation_min	(-10.f),
	m_particle_position_x_variation_max (10.f),
	m_particle_position_y_variation_min (0.f),
	m_particle_position_y_variation_max	(0.f),
	m_particle_position_z_variation_min	(-10.f),
	m_particle_position_z_variation_max	(10.f),
	m_position_x_distribution(m_particle_position_x_variation_min, m_particle_position_x_variation_max),
	m_position_y_distribution(m_particle_position_y_variation_min, m_particle_position_y_variation_max),
	m_position_z_distribution(m_particle_position_z_variation_min, m_particle_position_z_variation_max),

	m_particle_base_velocity_x	(0.0f),
	m_particle_base_velocity_y	(0.0f),
	m_particle_base_velocity_z	(0.0f),
	m_particle_velocity_x_variation_min	(-1.f),
	m_particle_velocity_x_variation_max (1.f),
	m_particle_velocity_y_variation_min	(-25.f),
	m_particle_velocity_y_variation_max	(-20.f),
	m_particle_velocity_z_variation_min	(-1.f),
	m_particle_velocity_z_variation_max	(1.f),
	m_velocity_x_distribution(m_particle_velocity_x_variation_min, m_particle_velocity_x_variation_max),
	m_velocity_y_distribution(m_particle_velocity_y_variation_min, m_particle_velocity_y_variation_max),
	m_velocity_z_distribution(m_particle_velocity_z_variation_min, m_particle_velocity_z_variation_max),

	m_is_color_randomized(false),
	m_color_distribution(0, 1),
	m_particle_color(1, 1, 1)
{
	CreateWICTextureFromFile(pp_device, L"Resources/Particles/Raindrop.png", 0, mcp_particle_texture_srv.GetAddressOf());

	D3D11_BLEND_DESC blend_desc = {};
	blend_desc.AlphaToCoverageEnable					= false;
	blend_desc.IndependentBlendEnable					= false;
	blend_desc.RenderTarget[0].BlendEnable				= true;
	blend_desc.RenderTarget[0].BlendOp					= D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].BlendOpAlpha				= D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].DestBlend				= D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlendAlpha			= D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
	blend_desc.RenderTarget[0].SrcBlend					= D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].SrcBlendAlpha			= D3D11_BLEND_ZERO;
	pp_device->CreateBlendState(&blend_desc, mcp_particle_blend_state.GetAddressOf());
	
	// Single object vertex set for instancing
	m_vertices.emplace_back(
		XMFLOAT4(0 - m_particle_half_width, 0 - m_particle_half_height, 0, 1), 
		XMFLOAT2(0, 0));
	m_vertices.emplace_back(
		XMFLOAT4(0 - m_particle_half_width, 0 + m_particle_half_height, 0, 1), 
		XMFLOAT2(0, 1));
	m_vertices.emplace_back(
		XMFLOAT4(0 + m_particle_half_width, 0 - m_particle_half_height, 0, 1), 
		XMFLOAT2(1, 0));
	m_vertices.emplace_back(
		XMFLOAT4(0 + m_particle_half_width, 0 + m_particle_half_height, 0, 1), 
		XMFLOAT2(1, 1));
	D3D11_BUFFER_DESC vertex_buffer_desc = {};
	vertex_buffer_desc.Usage				= D3D11_USAGE_IMMUTABLE;
	vertex_buffer_desc.ByteWidth			= sizeof(ParticleVertex) * 4;
	vertex_buffer_desc.BindFlags			= D3D11_BIND_VERTEX_BUFFER;
	vertex_buffer_desc.CPUAccessFlags		= 0;
	vertex_buffer_desc.MiscFlags			= 0;
	vertex_buffer_desc.StructureByteStride	= 0;
	D3D11_SUBRESOURCE_DATA vertices_data;
	vertices_data.pSysMem					= &m_vertices[0];
	vertices_data.SysMemPitch				= 0;
	vertices_data.SysMemSlicePitch			= 0;
	pp_device->CreateBuffer(&vertex_buffer_desc, &vertices_data, mcp_vertex_buffer.GetAddressOf());

	// Single object index set for instancing
	UINT* indices = new UINT[6];
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 2;
	indices[4] = 1;
	indices[5] = 3;
	D3D11_BUFFER_DESC index_buffer_desc = {};
	index_buffer_desc.Usage					= D3D11_USAGE_IMMUTABLE;
	index_buffer_desc.ByteWidth				= sizeof(UINT) * 6;
	index_buffer_desc.BindFlags				= D3D11_BIND_INDEX_BUFFER;
	index_buffer_desc.CPUAccessFlags		= 0;
	index_buffer_desc.MiscFlags				= 0;
	index_buffer_desc.StructureByteStride	= 0;
	D3D11_SUBRESOURCE_DATA indices_data;
	indices_data.pSysMem					= indices;
	indices_data.SysMemPitch				= 0;
	indices_data.SysMemSlicePitch			= 0;
	pp_device->CreateBuffer(&index_buffer_desc, &indices_data, mcp_index_buffer.GetAddressOf());
	delete[] indices;

	// Buffer for instances
	D3D11_BUFFER_DESC instance_buffer_desc = {};
	instance_buffer_desc.Usage				= D3D11_USAGE_DYNAMIC;
	instance_buffer_desc.ByteWidth			= sizeof(ParticleInstance) * m_max_particle_count;
	instance_buffer_desc.BindFlags			= D3D11_BIND_VERTEX_BUFFER;
	instance_buffer_desc.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
	instance_buffer_desc.MiscFlags			= 0;
	instance_buffer_desc.StructureByteStride = 0;
	pp_device->CreateBuffer(&instance_buffer_desc, NULL, mcp_instance_buffer.GetAddressOf());
}

ParticleEmitterComponent::~ParticleEmitterComponent()
{
}
