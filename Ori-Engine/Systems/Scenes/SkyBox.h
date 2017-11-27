#pragma once

#include <d3d11.h>
#include <memory>
#include <wrl.h>

class SkyBox
{
public:
	SkyBox();
	~SkyBox();

	void LoadEnvMap(ID3D11Device& pDevice, const wchar_t* pFilePath);
	void LoadIrradianceMap(ID3D11Device& pDevice, const wchar_t* pFilePath);

	void CreatePfemSrv(ID3D11Device* pp_device, ID3D11Texture2D* pp_T2d, D3D11_SHADER_RESOURCE_VIEW_DESC* pp_SrvDesc);
	void CreateBrdfLutSrv(ID3D11Device * pp_device, ID3D11Texture2D * pp_T2d, D3D11_SHADER_RESOURCE_VIEW_DESC * pp_SrvDesc);

	ID3D11ShaderResourceView* GetEnvMapSrv();
	ID3D11ShaderResourceView* GetIrradianceMapSrv();
	ID3D11ShaderResourceView* GetPfemSrv();
	ID3D11ShaderResourceView* GetBrdfLutSrv();

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_env_map_srv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_irradiance_map_srv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_pfem_srv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_brdf_lut_srv;
};

