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
		ID3D11SamplerState* pp_CurrentSampler, const ShadowRenderer& pr_ShadowRenderer, QuadRenderer& pr_QuadRenderer);
	~DeferredRenderer();

	ID3D11DepthStencilView* GetDepthDsv();

	void InitializeGBuffer();
	void PopulateGBuffers(const std::vector<std::unique_ptr<Entity>>& pr_EntityVector, const Entity& pr_camera);
	void CompositeShading(const Entity& pr_camera, const Entity& pr_Sun, SkyBox& pr_SkyBox, ID3D11RenderTargetView* pp_destination_rtv);

private:
	// References
	ID3D11Device* mp_device;
	ID3D11DeviceContext* mp_context;
	const unsigned int& mr_width;
	const unsigned int& mr_height;
	ID3D11SamplerState* mp_sampler_filtering_choice;
	const ShadowRenderer& mShadowRenderer;	// replace with reference to sampler collection?
	QuadRenderer& mr_quad_renderer;
	
	// Shaders
	std::unique_ptr<VertexShader> mup_deferred_buffer_vertex_shader;
	std::unique_ptr<PixelShader> mup_deferred_buffer_pixel_shader;
	std::unique_ptr<PixelShader> mup_deferred_buffer_material_test_pixel_shader;
	std::unique_ptr<PixelShader> mup_deferred_composite_pixel_shader;

	// Resources
	float m_clear_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_sampler_linear;

	static const int m_BUFFER_COUNT = 2;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mcp_gbuffer_textures[m_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mcp_gbuffer_rtvs[m_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_gbuffer_srvs[m_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mcp_depth_stencil_view;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_depth_srv;
};

