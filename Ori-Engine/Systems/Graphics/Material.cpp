#include "Material.h"

#include <WICTextureLoader.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

Material::Material()
{
}

Material::Material(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, const wchar_t * p_base_color_filepath, 
	const wchar_t * p_metalness_filepath, const wchar_t * p_roughness_filepath, const wchar_t * p_normal_filepath, const wchar_t * p_ambient_occlusion_filepath)
{
	CreateWICTextureFromFileEx(device.Get(), context.Get(), p_base_color_filepath, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, WIC_LOADER_FORCE_SRGB, nullptr, m_base_color_map.GetAddressOf());
	//CreateWICTextureFromFile(device.Get(), context.Get(), p_base_color_filepath, 0, m_base_color_map.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), p_metalness_filepath, 0, m_metalness_map.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), p_roughness_filepath, 0, m_roughness_map.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), p_normal_filepath, 0, m_normal_map.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), p_ambient_occlusion_filepath, 0, m_ambient_occlusion_map.GetAddressOf());
}

Material::Material(float p_metalness, float p_roughness, float p_opacity)
{
	m_is_test_material = true;
	m_metalness = p_metalness;
	m_roughness = p_roughness;
	m_opacity = p_opacity;
}

Material::~Material()
{
}

ID3D11ShaderResourceView* Material::GetBaseColorMap()
{
	return m_base_color_map.Get();
}

ID3D11ShaderResourceView* Material::GetMetalnessMap()
{
	return m_metalness_map.Get();
}

ID3D11ShaderResourceView* Material::GetRoughnessMap()
{
	return m_roughness_map.Get();
}

ID3D11ShaderResourceView* Material::GetNormalMap()
{
	return m_normal_map.Get();
}

ID3D11ShaderResourceView* Material::GetAmbientOcclusionMap()
{
	return m_ambient_occlusion_map.Get();
}

ID3D11ShaderResourceView* Material::GetOpacityMap()
{
	return m_opacity_map.Get();
}

bool Material::IsTestMaterial()
{
	return m_is_test_material;
}

float Material::GetTestMetalness()
{
	return m_metalness;
}

float Material::GetTestRoughness()
{
	return m_roughness;
}

float Material::GetTestOpacity()
{
	return m_opacity;
}

