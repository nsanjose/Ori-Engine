#pragma once

#include <DirectXMath.h>
#include <d3d11.h>

#include <memory>
#include <vector>
#include <wrl.h>

#include "Component.h"

class Light {
public:
	virtual ~Light() {};
	DirectX::XMFLOAT3 m_color;
	float m_irradiance;
};
class DirectionalLight : public Light{
public:
	DirectionalLight(DirectX::XMFLOAT3 p_color, float p_irradiance)
	{
		m_color = p_color;
		m_irradiance = p_irradiance;
	}
};
class PointLight : public Light {
public:
	PointLight(DirectX::XMFLOAT3 p_color, float p_irradiance, float p_radius)
	{
		m_color = p_color;
		m_irradiance = p_irradiance;
		m_radius = p_radius;
	}
	float m_radius;
};
class SpotLight : public Light {
public:
	SpotLight(DirectX::XMFLOAT3 p_color, float p_irradiance)
	{
		m_color = p_color;
		m_irradiance = p_irradiance;
	}
};

struct Shadow {
	virtual ~Shadow() {};
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDepthStencilView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowShaderResourceView;
	DirectX::XMFLOAT4X4 shadowViewMatrix;
	DirectX::XMFLOAT4X4 shadowProjectionMatrix;
};

static const UINT MAX_NUM_SHADOW_CASCADES = 5;	// MAX_NUM_SHADOW_CASCADES must have equal value to variable of the same name in shader

struct CascadedShadow : public Shadow {
	CascadedShadow(const Shadow& pShadow)
	{
		shadowDepthStencilView = pShadow.shadowDepthStencilView;
		shadowViewMatrix = pShadow.shadowViewMatrix;
	}
	UINT m_num_cascades;
	DirectX::XMFLOAT4X4 m_cascade_projections[MAX_NUM_SHADOW_CASCADES];
	float m_cascade_far_clips[MAX_NUM_SHADOW_CASCADES - 1];
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mcp_cascade_texture;
	std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> mcp_cascade_dsvs;
};

class LightComponent : public Component
{
public:
	LightComponent();
	LightComponent(std::unique_ptr<Light> _light);
	virtual ~LightComponent();

	Light* GetGenericLight();
	template<class T>
	T* GetLight()
	{
		return std::dynamic_pointer_cast<T>(light.get());
	};

	template<class T>
	T& GetShadowByType() const
	{
		const std::type_info& searchType = typeid(T);
		if (shadow && typeid(*mup_shadow.get()) == searchType)
		{
			return static_cast<T&>(*mup_shadow.get());
		}
	};

	std::unique_ptr<Shadow> mup_shadow;

private:
	std::unique_ptr<Light> light;
};
