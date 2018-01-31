#include "ParticleEmitterComponent.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

ParticleEmitterComponent::ParticleEmitterComponent(TransformComponent& pr_transform, ID3D11Device* pp_device) : Component(),
	m_time_since_emission		(0.0f),

	// Settings
	m_is_emitting				(true),
	m_emissions_per_second		(50.0f),
	m_max_particle_age			(10.0f),
	m_max_particle_count		(1000),
	m_particle_size				(0.05f),

	m_particle_position_x_variation_min	(-0.5f),
	m_particle_position_x_variation_max (0.5f),
	m_particle_position_y_variation_min (-0.5f),
	m_particle_position_y_variation_max	(0.5f),
	m_particle_position_z_variation_min	(-0.5f),
	m_particle_position_z_variation_max	(0.5f),
	m_position_x_distribution(m_particle_position_x_variation_min, m_particle_position_x_variation_max),
	m_position_y_distribution(m_particle_position_y_variation_min, m_particle_position_y_variation_max),
	m_position_z_distribution(m_particle_position_z_variation_min, m_particle_position_z_variation_max),

	m_particle_base_velocity_x	(0.0f),
	m_particle_base_velocity_y	(0.0f),
	m_particle_base_velocity_z	(0.0f),
	m_particle_velocity_x_variation_min	(-0.2f),
	m_particle_velocity_x_variation_max (0.2f),
	m_particle_velocity_y_variation_min	(-0.2f),
	m_particle_velocity_y_variation_max	(0.2f),
	m_particle_velocity_z_variation_min	(-0.2f),
	m_particle_velocity_z_variation_max	(0.2f),
	m_velocity_x_distribution(m_particle_velocity_x_variation_min, m_particle_velocity_x_variation_max),
	m_velocity_y_distribution(m_particle_velocity_y_variation_min, m_particle_velocity_y_variation_max),
	m_velocity_z_distribution(m_particle_velocity_z_variation_min, m_particle_velocity_z_variation_max),

	m_color_distribution(0, 1)
{
	// Single object vertex set for instancing
	m_vertices.emplace_back(XMFLOAT4(0 - m_particle_size, 0 - m_particle_size, 0, 1));
	m_vertices.emplace_back(XMFLOAT4(0 - m_particle_size, 0 + m_particle_size, 0, 1));
	m_vertices.emplace_back(XMFLOAT4(0 + m_particle_size, 0 - m_particle_size, 0, 1));
	m_vertices.emplace_back(XMFLOAT4(0 + m_particle_size, 0 + m_particle_size, 0, 1));
	D3D11_BUFFER_DESC vertex_buffer_desc = {};
	vertex_buffer_desc.Usage				= D3D11_USAGE_DYNAMIC;
	vertex_buffer_desc.ByteWidth			= sizeof(ParticleVertex) * 4;
	vertex_buffer_desc.BindFlags			= D3D11_BIND_VERTEX_BUFFER;
	vertex_buffer_desc.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
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
	index_buffer_desc.Usage					= D3D11_USAGE_DYNAMIC;
	index_buffer_desc.ByteWidth				= sizeof(UINT) * 6;
	index_buffer_desc.BindFlags				= D3D11_BIND_INDEX_BUFFER;
	index_buffer_desc.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
	index_buffer_desc.MiscFlags				= 0;
	index_buffer_desc.StructureByteStride	= 0;
	D3D11_SUBRESOURCE_DATA indices_data;
	indices_data.pSysMem					= indices;
	indices_data.SysMemPitch				= 0;
	indices_data.SysMemSlicePitch			= 0;
	pp_device->CreateBuffer(&index_buffer_desc, &indices_data, mcp_index_buffer.GetAddressOf());
	delete[] indices;
}

ParticleEmitterComponent::~ParticleEmitterComponent()
{
}
