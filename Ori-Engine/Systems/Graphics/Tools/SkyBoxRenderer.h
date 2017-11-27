#pragma once

#include <d3d11.h>
#include <memory>
#include <wrl.h>

#include "..\..\..\Components\CameraComponent.h"
#include "..\..\..\Components\DrawComponent.h"
#include "..\..\..\Systems\Scenes\Entity.h"
#include "..\Mesh.h"
#include "..\Shader.h"

class SkyBoxRenderer
{
public:
	SkyBoxRenderer(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context);
	~SkyBoxRenderer();

	void RenderStenciledSkyBox(ID3D11ShaderResourceView* pp_skybox_srv, const Entity& pr_camera, ID3D11DepthStencilView* pp_depth_stencil_view,
		ID3D11RenderTargetView* pp_destination_rtv);

private:
	// References
	ID3D11Device* mp_device;
	ID3D11DeviceContext* mp_context;

	// Shaders
	std::unique_ptr<VertexShader> mup_skybox_vertex_shader;
	std::unique_ptr<PixelShader> mup_skybox_pixel_shader;

	// Resources
	std::unique_ptr<Mesh> mup_cube_mesh;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> mcp_skybox_rasterizer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mcp_skybox_depth_stencil_state;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_skybox_sampler;
};

