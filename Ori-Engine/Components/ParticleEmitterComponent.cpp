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
}

ParticleEmitterComponent::~ParticleEmitterComponent()
{
}
