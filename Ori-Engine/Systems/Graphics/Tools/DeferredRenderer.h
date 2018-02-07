#pragma once

#include <wrl.h>
#include <vector>
#include <d3d11.h>

#include "..\..\..\Components\DrawComponent.h"
#include "..\..\..\Components\LightComponent.h"
#include "..\..\..\Systems\Scenes\Entity.h"
#include "..\..\..\Systems\Scenes\SkyBox.h"
#include "..\Tools\ShadowRenderer.h"
#include "..\Tools\QuadRenderer.h"
#include "..\Shader.h"

class DeferredRenderer
{
public:
	DeferredRenderer(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, const unsigned int& pr_Width, const unsigned int& pr_Height,
		const ShadowRenderer& pr_ShadowRenderer, QuadRenderer& pr_QuadRenderer);
	~DeferredRenderer();

	void InitializeGBuffer();
	void ClearAllGBuffers();
	void ClearShadowBuffer();
	void PopulateShadowBuffer(const Entity& pr_light, const std::vector<std::unique_ptr<Entity>>& pp_occluders, const Entity& pr_camera);
	void PopulateGBuffers(const std::vector<std::unique_ptr<Entity>>& pr_EntityVector, const Entity& pr_camera);
	void CompositeShading(const Entity& pr_camera, const Entity& pr_Sun, ID3D11RenderTargetView* pp_destination_rtv);
	void ApplyIBL(const Entity& pr_camera, SkyBox* pp_SkyBox, ID3D11RenderTargetView* pp_destination_rtv);

	ID3D11DepthStencilView* GetDepthDsv();

private:
	// References
	ID3D11Device* mp_device;
	ID3D11DeviceContext* mp_context;
	const unsigned int& mr_width;
	const unsigned int& mr_height;
	ID3D11SamplerState* mp_sampler_filtering_choice;
	const ShadowRenderer& m_shadow_renderer;	// replace with reference to sampler collection?
	QuadRenderer& mr_quad_renderer;
	
	// Shaders
	std::unique_ptr<PixelShader> mup_deferred_clear_gbuffer_pixel_shader;
	std::unique_ptr<VertexShader> mup_deferred_buffer_vertex_shader;
	std::unique_ptr<PixelShader> mup_deferred_buffer_pixel_shader;
	std::unique_ptr<PixelShader> mup_deferred_buffer_material_test_pixel_shader;
	std::unique_ptr<VertexShader> mup_deferred_shadow_buffer_vertex_shader;
	std::unique_ptr<PixelShader> mup_deferred_shadow_buffer_pixel_shader;
	std::unique_ptr<PixelShader> mup_deferred_composite_pixel_shader;
	std::unique_ptr<PixelShader> mup_deferred_apply_ibl_pixel_shader;

	// Resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_sampler_point;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_sampler_bilinear;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_sampler_ansio;
	Microsoft::WRL::ComPtr<ID3D11BlendState> mcp_clear_shadow_buffer_blend_state;
	Microsoft::WRL::ComPtr<ID3D11BlendState> mcp_shadow_buffer_blend_state;
	Microsoft::WRL::ComPtr<ID3D11BlendState> mcp_gbuffer_blend_state;
	Microsoft::WRL::ComPtr<ID3D11BlendState> mcp_accumulate_shading_blend_state;

	static const int m_BUFFER_COUNT = 3;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mcp_gbuffer_textures[m_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mcp_gbuffer_rtvs[m_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_gbuffer_srvs[m_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mcp_depth_stencil_view;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_depth_srv;
	float m_gbuffer_0_clear_color[4] = { 1, 1, 1, 1 };
	float m_gbuffer_1_clear_color[4] = { 1, 1, 1, 1 };
	float m_gbuffer_2_clear_color[4] = { 1, 1, 1, 1 };
};

