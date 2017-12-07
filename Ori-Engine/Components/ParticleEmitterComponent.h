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
	DirectX::XMFLOAT4 m_color;
	DirectX::XMFLOAT3 m_velocity;
	float m_age;
};

struct ParticleVertex
{
	DirectX::XMFLOAT4 m_position;
	//DirectX::XMFLOAT2 tex_coord;
	DirectX::XMFLOAT4 m_color;
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
	float m_particle_size;

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
	std::uniform_real_distribution<float> m_color_distribution;

	// Buffer resources
	std::vector<Particle> m_particles;
	std::vector<ParticleVertex> m_vertices;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mcp_vertex_buffer, mcp_index_buffer;
};

