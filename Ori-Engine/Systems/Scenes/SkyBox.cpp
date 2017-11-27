#include "SkyBox.h"

#include <DDSTextureLoader.h>

SkyBox::SkyBox()
{
}

SkyBox::~SkyBox()
{
}

void SkyBox::LoadEnvMap(ID3D11Device& pDevice, const wchar_t* pFilePath)
{
	//DirectX::CreateDDSTextureFromFile(&pDevice, pFilePath, nullptr, mcp_env_map_srv.GetAddressOf());
	DirectX::CreateDDSTextureFromFileEx(&pDevice, pFilePath, 0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, true, nullptr, mcp_env_map_srv.GetAddressOf());
}

void SkyBox::LoadIrradianceMap(ID3D11Device& pDevice, const wchar_t* pFilePath)
{
	//DirectX::CreateDDSTextureFromFile(&pDevice, pFilePath, nullptr, mcp_irradiance_map_srv.GetAddressOf());
	DirectX::CreateDDSTextureFromFileEx(&pDevice, pFilePath, 0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, true, nullptr, mcp_irradiance_map_srv.GetAddressOf());
}

ID3D11ShaderResourceView* SkyBox::GetEnvMapSrv()
{
	return mcp_env_map_srv.Get();
}

ID3D11ShaderResourceView* SkyBox::GetIrradianceMapSrv()
{
	return mcp_irradiance_map_srv.Get();
}

ID3D11ShaderResourceView* SkyBox::GetPfemSrv()
{
	return mcp_pfem_srv.Get();
}

ID3D11ShaderResourceView* SkyBox::GetBrdfLutSrv()
{
	return mcp_brdf_lut_srv.Get();
}

void SkyBox::CreatePfemSrv(ID3D11Device* pp_device, ID3D11Texture2D * pp_T2d, D3D11_SHADER_RESOURCE_VIEW_DESC * pp_SrvDesc)
{
	pp_device->CreateShaderResourceView(pp_T2d, pp_SrvDesc, mcp_pfem_srv.GetAddressOf());
}

void SkyBox::CreateBrdfLutSrv(ID3D11Device* pp_device, ID3D11Texture2D * pp_T2d, D3D11_SHADER_RESOURCE_VIEW_DESC * pp_SrvDesc)
{
	pp_device->CreateShaderResourceView(pp_T2d, pp_SrvDesc, mcp_brdf_lut_srv.GetAddressOf());
}


