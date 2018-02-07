#pragma once

#include <random>
#include <vector>
#include <wrl.h>

#include <d3d11.h>
#include <DirectXMath.h>

#include "Component.h"
#include "TransformComponent.h"

struct Particle
{
	DirectX::XMFLOAT4 m_position = { 0, 0, 0, 1 };
	DirectX::XMFLOAT3 m_velocity;
	float m_age = .0f;
};

struct ParticleVertex
{
	ParticleVertex(const DirectX::XMFLOAT4& p_position, const DirectX::XMFLOAT2 p_tex_coord) { 
		m_position = p_position;
		m_tex_coord = p_tex_coord;
	}
	DirectX::XMFLOAT4 m_position;
	DirectX::XMFLOAT2 m_tex_coord;
};

struct ParticleInstance
{
	DirectX::XMFLOAT4 m_color;
	DirectX::XMFLOAT4X4 m_world_matrix;
};

class ParticleEmitterComponent : public Component
{
public:
	ParticleEmitterComponent(TransformComponent& pr_transform, ID3D11Device* pp_device);
	~ParticleEmitterComponent();

	bool m_is_emitting;
	float m_time_since_emission;
	float m_emissions_per_second;
	unsigned int m_max_particle_count;
	float m_max_particle_age;
	float m_particle_half_width;
	float m_particle_half_height;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_particle_texture_srv;
	Microsoft::WRL::ComPtr<ID3D11BlendState> mcp_particle_blend_state;

	// Spawn variations
	// Position variation
	float m_particle_position_x_variation_min;
	float m_particle_position_x_variation_max;
	float m_particle_position_y_variation_min;
	float m_particle_position_y_variation_max;
	float m_particle_position_z_variation_min;
	float m_particle_position_z_variation_max;
	std::uniform_real_distribution<float> m_position_x_distribution;
	std::uniform_real_distribution<float> m_position_y_distribution;
	std::uniform_real_distribution<float> m_position_z_distribution;

	// Velocity variation
	float m_particle_base_velocity_x;
	float m_particle_base_velocity_y;
	float m_particle_base_velocity_z;
	float m_particle_velocity_x_variation_min;
	float m_particle_velocity_x_variation_max;
	float m_particle_velocity_y_variation_min;
	float m_particle_velocity_y_variation_max;
	float m_particle_velocity_z_variation_min;
	float m_particle_velocity_z_variation_max;
	std::uniform_real_distribution<float> m_velocity_x_distribution;
	std::uniform_real_distribution<float> m_velocity_y_distribution;
	std::uniform_real_distribution<float> m_velocity_z_distribution;

	// Color variation
	bool m_is_color_randomized;
	std::uniform_real_distribution<float> m_color_distribution;
	DirectX::XMFLOAT3 m_particle_color;

	// Buffer resources
	std::vector<Particle> m_particles;
	std::vector<ParticleVertex> m_vertices;
	std::vector<ParticleInstance> m_instances;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mcp_vertex_buffer, mcp_index_buffer, mcp_instance_buffer;
};

