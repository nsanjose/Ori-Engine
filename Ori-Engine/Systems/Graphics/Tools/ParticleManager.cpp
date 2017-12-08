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

	D3D11_BLEND_DESC blend_desc							= {};
	blend_desc.AlphaToCoverageEnable					= false;
	blend_desc.IndependentBlendEnable					= false;
	blend_desc.RenderTarget[0].BlendEnable				= true;
	blend_desc.RenderTarget[0].SrcBlend					= D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlend				= D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].BlendOp					= D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha			= D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlendAlpha			= D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].BlendOpAlpha				= D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
	mp_device->CreateBlendState(&blend_desc, mcp_blend_state.GetAddressOf());
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

		unsigned int stride = sizeof(ParticleVertex);
		unsigned int offset = 0;
		mp_context->IASetVertexBuffers(0, 1, emitter.mcp_vertex_buffer.GetAddressOf(), &stride, &offset);
		mp_context->IASetIndexBuffer(emitter.mcp_index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		mp_context->DrawIndexed(emitter.m_particles.size() * 6, 0, 0);
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

				// remove vertices
				emitter.m_vertices.erase(emitter.m_vertices.begin() + i, emitter.m_vertices.begin() + i + 4);
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

					// Variate spawn position
					emitter.m_particles.back().m_translation.x = emitter.m_position_x_distribution(rng);
					emitter.m_particles.back().m_translation.y = emitter.m_position_y_distribution(rng);
					emitter.m_particles.back().m_translation.z = emitter.m_position_z_distribution(rng);

					// Variate color
					emitter.m_particles.back().m_color.x = emitter.m_color_distribution(rng);
					emitter.m_particles.back().m_color.y = emitter.m_color_distribution(rng);
					emitter.m_particles.back().m_color.z = emitter.m_color_distribution(rng);
					emitter.m_particles.back().m_color.w = 1.0f;

					// Variate velocity
					emitter.m_particles.back().m_velocity.x = emitter.m_particle_base_velocity_x + emitter.m_velocity_x_distribution(rng);
					emitter.m_particles.back().m_velocity.y = emitter.m_particle_base_velocity_y + emitter.m_velocity_y_distribution(rng);
					emitter.m_particles.back().m_velocity.z = emitter.m_particle_base_velocity_z + emitter.m_velocity_z_distribution(rng);

					emitter.m_particles.back().m_age = 0.0f;
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

			// Update translation
			emitter.m_particles[i].m_translation.x += emitter.m_particles[i].m_velocity.x * p_delta_time;
			emitter.m_particles[i].m_translation.y += emitter.m_particles[i].m_velocity.y * p_delta_time;
			emitter.m_particles[i].m_translation.z += emitter.m_particles[i].m_velocity.z * p_delta_time;
		}
	}
}

void ParticleManager::UpdateBuffers(std::vector<Entity*>& pr_entities)
{
	for (Entity* entity : pr_entities)
	{
		if (!(entity->HasComponent<ParticleEmitterComponent>())) { continue; }
		ParticleEmitterComponent& emitter = *entity->GetComponentByType<ParticleEmitterComponent>();

		// Resize vertex vector if too large/small
		while (emitter.m_vertices.size() > (emitter.m_particles.size() * 4)) { emitter.m_vertices.pop_back(); }
		while (emitter.m_vertices.size() < (emitter.m_particles.size() * 4)) { emitter.m_vertices.emplace_back(); }
		
		// Update vertex vector
		unsigned int vertex_i = 0;
		for (int particle_i = 0; particle_i < emitter.m_particles.size(); particle_i++)
		{
			// Bottom Left
			emitter.m_vertices[vertex_i].m_position.x = 0 - emitter.m_particle_size;
			emitter.m_vertices[vertex_i].m_position.y = 0 - emitter.m_particle_size;
			emitter.m_vertices[vertex_i].m_position.z = 0;
			emitter.m_vertices[vertex_i].m_position.w = 1;
			emitter.m_vertices[vertex_i].m_color = emitter.m_particles[particle_i].m_color;
			XMMATRIX temp_translation_matrix = XMMatrixTranslationFromVector(XMLoadFloat3(&(emitter.m_particles[particle_i].m_translation)));
			XMStoreFloat4x4(&(emitter.m_vertices[vertex_i].m_world_matrix), XMMatrixTranspose(temp_translation_matrix * XMMatrixTranspose(XMLoadFloat4x4(&(entity->GetTransformComponent().m_world_matrix)))));
			vertex_i++;

			// Top Left
			emitter.m_vertices[vertex_i].m_position.x = 0 - emitter.m_particle_size;
			emitter.m_vertices[vertex_i].m_position.y = 0 + emitter.m_particle_size;
			emitter.m_vertices[vertex_i].m_position.z = 0;
			emitter.m_vertices[vertex_i].m_position.w = 1;
			emitter.m_vertices[vertex_i].m_color = emitter.m_particles[particle_i].m_color;
			temp_translation_matrix = XMMatrixTranslationFromVector(XMLoadFloat3(&(emitter.m_particles[particle_i].m_translation)));
			XMStoreFloat4x4(&(emitter.m_vertices[vertex_i].m_world_matrix), XMMatrixTranspose(temp_translation_matrix * XMMatrixTranspose(XMLoadFloat4x4(&(entity->GetTransformComponent().m_world_matrix)))));
			vertex_i++;

			// Bottom Right
			emitter.m_vertices[vertex_i].m_position.x = 0 + emitter.m_particle_size;
			emitter.m_vertices[vertex_i].m_position.y = 0 - emitter.m_particle_size;
			emitter.m_vertices[vertex_i].m_position.z = 0;
			emitter.m_vertices[vertex_i].m_position.w = 1;
			emitter.m_vertices[vertex_i].m_color = emitter.m_particles[particle_i].m_color;
			temp_translation_matrix = XMMatrixTranslationFromVector(XMLoadFloat3(&(emitter.m_particles[particle_i].m_translation)));
			XMStoreFloat4x4(&(emitter.m_vertices[vertex_i].m_world_matrix), XMMatrixTranspose(temp_translation_matrix * XMMatrixTranspose(XMLoadFloat4x4(&(entity->GetTransformComponent().m_world_matrix)))));
			vertex_i++;

			// Top Right
			emitter.m_vertices[vertex_i].m_position.x = 0 + emitter.m_particle_size;
			emitter.m_vertices[vertex_i].m_position.y = 0 + emitter.m_particle_size;
			emitter.m_vertices[vertex_i].m_position.z = 0;
			emitter.m_vertices[vertex_i].m_position.w = 1;
			emitter.m_vertices[vertex_i].m_color = emitter.m_particles[particle_i].m_color;
			temp_translation_matrix = XMMatrixTranslationFromVector(XMLoadFloat3(&(emitter.m_particles[particle_i].m_translation)));
			XMStoreFloat4x4(&(emitter.m_vertices[vertex_i].m_world_matrix), XMMatrixTranspose(temp_translation_matrix * XMMatrixTranspose(XMLoadFloat4x4(&(entity->GetTransformComponent().m_world_matrix)))));
			vertex_i++;
		}

		// Create buffers
		if (emitter.m_particles.size() > 0)
		{
			// Vertices
			D3D11_BUFFER_DESC vertex_buffer_desc = {};
			vertex_buffer_desc.Usage				= D3D11_USAGE_DYNAMIC;
			vertex_buffer_desc.ByteWidth			= sizeof(ParticleVertex) * 4 * emitter.m_particles.size();
			vertex_buffer_desc.BindFlags			= D3D11_BIND_VERTEX_BUFFER;
			vertex_buffer_desc.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
			vertex_buffer_desc.MiscFlags			= 0;
			vertex_buffer_desc.StructureByteStride	= 0;
			D3D11_SUBRESOURCE_DATA vertices_data;
			vertices_data.pSysMem = &(emitter.m_vertices[0]);
			vertices_data.SysMemPitch = 0;
			vertices_data.SysMemSlicePitch = 0;
			mp_device->CreateBuffer(&vertex_buffer_desc, &vertices_data, emitter.mcp_vertex_buffer.ReleaseAndGetAddressOf());

			// Indices
			unsigned int* indices = new unsigned int[emitter.m_particles.size() * 6];
			unsigned int index_i = 0;
			for (unsigned int i = 0; i < emitter.m_particles.size(); i++)
			{
				indices[index_i++] = 0 + (i * 4);
				indices[index_i++] = 1 + (i * 4);
				indices[index_i++] = 2 + (i * 4);
				indices[index_i++] = 2 + (i * 4);
				indices[index_i++] = 1 + (i * 4);
				indices[index_i++] = 3 + (i * 4);
			}
			D3D11_BUFFER_DESC index_buffer_desc = {};
			index_buffer_desc.Usage					= D3D11_USAGE_DYNAMIC;
			index_buffer_desc.ByteWidth				= sizeof(unsigned int) * 6 * emitter.m_particles.size();
			index_buffer_desc.BindFlags				= D3D11_BIND_INDEX_BUFFER;
			index_buffer_desc.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
			index_buffer_desc.MiscFlags				= 0;
			index_buffer_desc.StructureByteStride	= 0;
			D3D11_SUBRESOURCE_DATA indices_data;
			indices_data.pSysMem			= indices;
			indices_data.SysMemPitch		= 0;
			indices_data.SysMemSlicePitch	= 0;
			mp_device->CreateBuffer(&index_buffer_desc, &indices_data, emitter.mcp_index_buffer.ReleaseAndGetAddressOf());
			delete[] indices;
		}
	}
}
