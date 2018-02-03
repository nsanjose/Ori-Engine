#include "ParticleManager.h"

#include <chrono>
#include <random>

using namespace DirectX;

ParticleManager::ParticleManager(ID3D11Device * pp_device, ID3D11DeviceContext * pp_context)
	: mp_device(pp_device), mp_context(pp_context)
{
	mup_particle_vertex_shader = std::make_unique<VertexShader>(mp_device, mp_context);
	if (!mup_particle_vertex_shader->InitializeShaderFromFile(L"x64/Debug/particle_vertex.cso"))
		mup_particle_vertex_shader->InitializeShaderFromFile(L"particle_vertex.cso");
	mup_particle_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_particle_pixel_shader->InitializeShaderFromFile(L"x64/Debug/particle_pixel.cso"))
		mup_particle_pixel_shader->InitializeShaderFromFile(L"particle_pixel.cso");

	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};
	depth_stencil_desc.DepthEnable				= true;
	depth_stencil_desc.DepthWriteMask			= D3D11_DEPTH_WRITE_MASK_ZERO;
	depth_stencil_desc.DepthFunc				= D3D11_COMPARISON_LESS;
	mp_device->CreateDepthStencilState(&depth_stencil_desc, mcp_depth_stencil_state.GetAddressOf());

	D3D11_SAMPLER_DESC sampler_bilinear_desc = {};
	ZeroMemory(&sampler_bilinear_desc, sizeof(D3D11_SAMPLER_DESC));
	sampler_bilinear_desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampler_bilinear_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_bilinear_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_bilinear_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	mp_device->CreateSamplerState(&sampler_bilinear_desc, mcp_sampler_bilinear.GetAddressOf());
}

ParticleManager::~ParticleManager()
{
}

std::vector<Entity*> ParticleManager::CollectEntitiesWithEmitters(std::vector<std::unique_ptr<Entity>>& pup_entities)
{
	std::vector<Entity*> emitter_entities;
	for (int i = 0; i < pup_entities.size(); i++)
	{
		if (pup_entities[i].get()->HasComponent<ParticleEmitterComponent>())
		{
			emitter_entities.push_back(pup_entities[i].get());
		}
	}
	return emitter_entities;
}

void ParticleManager::Update(std::vector<Entity*>& pp_entities, float p_delta_time)
{
	KillParticles(pp_entities, p_delta_time);
	EmitParticles(pp_entities, p_delta_time);
	UpdateParticles(pp_entities, p_delta_time);
	UpdateBuffers(pp_entities);
}

void ParticleManager::Render(const CameraComponent& pr_camera, const std::vector<Entity*>& pp_entities, ID3D11RenderTargetView* pp_rtv, ID3D11DepthStencilView* pp_dsv)
{
	mup_particle_vertex_shader->SetShader();
	mup_particle_pixel_shader->SetShader();

	mp_context->OMSetRenderTargets(1, &pp_rtv, pp_dsv);

	mup_particle_vertex_shader->SetConstantBufferMatrix4x4("view_matrix", pr_camera.m_view_matrix);
	mup_particle_vertex_shader->SetConstantBufferMatrix4x4("projection_matrix", pr_camera.m_projection_matrix);
	mup_particle_vertex_shader->UpdateAllConstantBuffers();

	for (Entity* entity : pp_entities)
	{
		if (!(entity->HasComponent<ParticleEmitterComponent>())) { continue; }
		ParticleEmitterComponent& emitter = *entity->GetComponentByType<ParticleEmitterComponent>();
		if (emitter.m_particles.size() <= 0) { continue; }

		mup_particle_pixel_shader->SetSamplerState("sampler_bilinear", mcp_sampler_bilinear.Get());
		mup_particle_pixel_shader->SetShaderResourceView("particle_texture", emitter.mcp_particle_texture_srv.Get());

		ID3D11Buffer* vertex_and_instance_buffers[2] = { emitter.mcp_vertex_buffer.Get(), emitter.mcp_instance_buffer.Get() };
		UINT strides[2] = { sizeof(ParticleVertex), sizeof(ParticleInstance) };
		UINT offsets[2] = { 0, 0 };
		mp_context->IASetVertexBuffers(0, 2, vertex_and_instance_buffers, strides, offsets);
		mp_context->IASetIndexBuffer(emitter.mcp_index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		mp_context->OMSetBlendState(emitter.mcp_particle_blend_state.Get(), 0, 0xffffffff);
		mp_context->DrawIndexedInstanced(6, emitter.m_particles.size(), 0, 0, 0);

		// Unbind
		mup_particle_pixel_shader->SetSamplerState("sampler_bilinear", 0);
		mup_particle_pixel_shader->SetShaderResourceView("particle_texture", 0);
		mp_context->OMSetBlendState(0, 0, 0xffffffff);
	}
}

void ParticleManager::KillParticles(std::vector<Entity*>& pr_entities, float p_delta_time)
{
	for (Entity* entity : pr_entities)
	{
		if (!(entity->HasComponent<ParticleEmitterComponent>())) { continue; }
		ParticleEmitterComponent& emitter = *entity->GetComponentByType<ParticleEmitterComponent>();

		for (unsigned int i = 0; i < emitter.m_particles.size(); i++)
		{
			// kill on age threshold
			if (emitter.m_particles[i].m_age > emitter.m_max_particle_age)
			{
				// remove particle
				emitter.m_particles.erase(emitter.m_particles.begin()) + i;
				emitter.m_instances.erase(emitter.m_instances.begin() + i);
			}
		}
	}
}

void ParticleManager::EmitParticles(std::vector<Entity*>& pr_entities, float p_delta_time)
{
	// Random number generator for particle variations
	std::mt19937_64 rng;
	rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());

	for (Entity* entity : pr_entities)
	{
		if (!(entity->HasComponent<ParticleEmitterComponent>())) { continue; }
		ParticleEmitterComponent& emitter = *entity->GetComponentByType<ParticleEmitterComponent>();

		// Deciding when to emit
		emitter.m_time_since_emission += p_delta_time;
		if (emitter.m_is_emitting && (emitter.m_time_since_emission > (1 / emitter.m_emissions_per_second)) && (emitter.m_particles.size() < emitter.m_max_particle_count))
		{
			while (emitter.m_time_since_emission > 0.0f)	// Un-cap spawn rate from framerate
			{
				if (emitter.m_particles.size() < emitter.m_max_particle_count)
				{
					// Add particle
					emitter.m_particles.emplace_back();
					emitter.m_instances.emplace_back();

					// Variate spawn position
					XMMATRIX spawn_offset = XMMatrixTranslationFromVector({
						emitter.m_position_x_distribution(rng),
						emitter.m_position_y_distribution(rng),
						emitter.m_position_z_distribution(rng) });
					XMStoreFloat4x4(&emitter.m_instances.back().m_world_matrix, XMMatrixTranspose(spawn_offset + XMMatrixTranspose(XMLoadFloat4x4(&entity->GetTransformComponent().m_world_matrix))));

					// Variate velocity
					emitter.m_particles.back().m_velocity.x = emitter.m_particle_base_velocity_x + emitter.m_velocity_x_distribution(rng);
					emitter.m_particles.back().m_velocity.y = emitter.m_particle_base_velocity_y + emitter.m_velocity_y_distribution(rng);
					emitter.m_particles.back().m_velocity.z = emitter.m_particle_base_velocity_z + emitter.m_velocity_z_distribution(rng);

					if (emitter.m_is_color_randomized)
					{
						// Variate color
						emitter.m_instances.back().m_color = {
							emitter.m_color_distribution(rng),
							emitter.m_color_distribution(rng),
							emitter.m_color_distribution(rng),
							1.0f };
					}
					else
					{
						emitter.m_instances.back().m_color = {
							emitter.m_particle_color.x,
							emitter.m_particle_color.y,
							emitter.m_particle_color.z,
							1.0f };
					}
				}

				emitter.m_time_since_emission -= (1 / emitter.m_emissions_per_second);
			}
		}
	}
}

void ParticleManager::UpdateParticles(std::vector<Entity*>& pr_entities, float p_delta_time)
{
	for (Entity* entity : pr_entities)
	{
		if (!(entity->HasComponent<ParticleEmitterComponent>())) { continue; }
		ParticleEmitterComponent& emitter = *entity->GetComponentByType<ParticleEmitterComponent>();

		for (unsigned int i = 0; i < emitter.m_particles.size(); i++)
		{
			// Update age
			emitter.m_particles[i].m_age += p_delta_time;

			// Update position
			XMMATRIX translation_matrix = XMMatrixTranslationFromVector({
				emitter.m_particles[i].m_velocity.x * p_delta_time,
				emitter.m_particles[i].m_velocity.y * p_delta_time,
				emitter.m_particles[i].m_velocity.z * p_delta_time} );
			XMStoreFloat4x4(&emitter.m_instances[i].m_world_matrix, XMMatrixTranspose(translation_matrix * XMMatrixTranspose(XMLoadFloat4x4(&emitter.m_instances[i].m_world_matrix))));
		}
	}
}

void ParticleManager::UpdateBuffers(std::vector<Entity*>& pr_entities)
{
	for (Entity* entity : pr_entities)
	{
		if (!(entity->HasComponent<ParticleEmitterComponent>())) { continue; }
		ParticleEmitterComponent& emitter = *entity->GetComponentByType<ParticleEmitterComponent>();

		if (emitter.m_particles.size() > 0)
		{
			D3D11_MAPPED_SUBRESOURCE instance_data;
			mp_context->Map(emitter.mcp_instance_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &instance_data);
			memcpy(instance_data.pData, &emitter.m_instances[0], sizeof(ParticleInstance) * emitter.m_instances.size());
			mp_context->Unmap(emitter.mcp_instance_buffer.Get(), 0);
		}
	}
}
