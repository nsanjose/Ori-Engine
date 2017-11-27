#pragma once

#include <d3d11.h>
#include <wrl.h>

#include "Shader.h"

class Material
{
public:
	Material();
	Material(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		const wchar_t* p_base_color_filepath, const wchar_t* p_metalness_filepath, const wchar_t* p_roughness_filepath, const wchar_t* p_normal_filepath, const wchar_t* p_ambient_occlusion_filepath);
	Material(float p_metalness, float p_roughness, float p_opacity);
	~Material();
	
	// Attach individual textures for flyweight

	ID3D11ShaderResourceView* GetBaseColorMap();
	ID3D11ShaderResourceView* GetMetalnessMap();
	ID3D11ShaderResourceView* GetRoughnessMap();
	ID3D11ShaderResourceView* GetNormalMap();
	ID3D11ShaderResourceView* GetAmbientOcclusionMap();
	ID3D11ShaderResourceView* GetOpacityMap();

	bool IsTestMaterial();
	float GetTestMetalness();
	float GetTestRoughness();
	float GetTestOpacity();

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_base_color_map;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_metalness_map;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_roughness_map;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normal_map;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ambient_occlusion_map;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_opacity_map;

	bool m_is_test_material = false;
	float m_metalness;
	float m_roughness;
	float m_opacity;
};

