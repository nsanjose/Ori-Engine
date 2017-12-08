#pragma once

#include <memory>
#include <wrl.h>

#include <d3d11.h>
#include <DirectXMath.h>

#include "..\..\..\Components\CameraComponent.h"
#include "..\..\..\Components\ParticleEmitterComponent.h"
#include "..\..\Scenes\Entity.h"
#include "..\Shader.h"

class ParticleManager
{
public:
	ParticleManager(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context);
	~ParticleManager();
	
	std::vector<Entity*> CollectEntitiesWithEmitters(std::vector<std::unique_ptr<Entity>>& pup_entities);
	void Update(std::vector<Entity*>& pp_entities, float p_delta_time);
	void Render(const CameraComponent& pr_camera, const std::vector<Entity*>& pp_entities, ID3D11RenderTargetView* pp_rtv, ID3D11DepthStencilView* pp_dsv);

private:
	// References
	ID3D11Device* mp_device;
	ID3D11DeviceContext* mp_context;

	// Shaders
	std::unique_ptr<VertexShader> mup_particle_vertex_shader;
	std::unique_ptr<PixelShader> mup_particle_pixel_shader;

	// Resources
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mcp_depth_stencil_state;
	Microsoft::WRL::ComPtr<ID3D11BlendState> mcp_blend_state;

	// Update loop
	void KillParticles(std::vector<Entity*>& pr_entities, float p_delta_time);
	void EmitParticles(std::vector<Entity*>& pr_entities, float p_delta_time);
	void UpdateParticles(std::vector<Entity*>& pr_entities, float p_delta_time);
	void UpdateBuffers(std::vector<Entity*>& pr_entities);
};

