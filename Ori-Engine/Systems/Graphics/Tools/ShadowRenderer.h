#pragma once

#include <DirectXMath.h>
#include <memory>
#include <wrl.h>

#include "..\..\..\Components\CameraComponent.h"
#include "..\..\..\Components\DrawComponent.h"
#include "..\..\..\Components\LightComponent.h"
#include "..\..\..\Systems\Scenes\Entity.h"
#include "..\Shader.h"

class ShadowRenderer
{
public:
	ShadowRenderer(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context);
	~ShadowRenderer();

	const Microsoft::WRL::ComPtr<ID3D11SamplerState>& GetSampler() const;

	void EnableShadowing(Entity& pLight);
	void EnableCascadedShadowing(Entity& pLight, UINT p_num_cascades);

	void CalculateCascadeProjections(Entity& pLightEntity, const Entity& pCameraEntity);
	void RenderCascadeShadowMap(const std::vector<std::unique_ptr<Entity>>& pup_draw_entities, Entity& pLightEntity, const Entity& pCameraEntity);

private:
	// References
	ID3D11Device* mp_device;
	ID3D11DeviceContext* mp_context;

	// Shaders
	std::unique_ptr<VertexShader> mup_shadow_vertex_shader;

	// Resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_shadow_sampler;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> mcp_shadow_rasterizer;
	const float m_SHADOW_MAP_SIZE = 1024;
};

