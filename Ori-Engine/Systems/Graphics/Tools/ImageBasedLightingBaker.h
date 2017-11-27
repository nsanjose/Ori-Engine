#pragma once

#include <d3d11.h>
#include <memory>
#include <wrl.h>

#include "..\..\..\Systems\Scenes\SkyBox.h"
#include "..\Tools\QuadRenderer.h"
#include "..\Shader.h"

class ImageBasedLightingBaker
{
public:
	ImageBasedLightingBaker(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, QuadRenderer& pr_quad_renderer);
	~ImageBasedLightingBaker();

	// void GenerateIrradianceMap();
	void GeneratePfem(SkyBox* pp_skybox);
	void GenerateBrdfLut(SkyBox* pp_skybox);

private:
	// References
	ID3D11Device* mp_device;
	ID3D11DeviceContext* mp_context;
	QuadRenderer& mr_quad_renderer;

	// Shaders
	std::unique_ptr<PixelShader> mup_bake_pfem_pixel_shader;
	std::unique_ptr<PixelShader> mup_bake_brdf_lut_pixel_shader;

	// Resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_sampler_ansio;
};

