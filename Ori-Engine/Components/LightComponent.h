#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <wrl.h>

#include "Component.h"

class Light {		// why do these need to be class (not struct) for template?
public:
	virtual ~Light() {};
	DirectX::XMFLOAT4 ambientColor;
	DirectX::XMFLOAT4 diffuseColor;
};
class DirectionalLight : public Light{
public:
	DirectionalLight(DirectX::XMFLOAT4 _aC, DirectX::XMFLOAT4 _dC)
	{
		ambientColor = _aC;
		diffuseColor = _dC;
	}
};
struct DirLightExport {
	DirectX::XMFLOAT4 ambientColor;
	DirectX::XMFLOAT4 diffuseColor;
	DirectX::XMFLOAT3 direction;
};
class PointLight : public Light {
};
struct PointLightExport {
	DirectX::XMFLOAT4 ambientColor;
	DirectX::XMFLOAT4 diffuseColor;
	DirectX::XMFLOAT3 position;
};
class SpotLight : public Light {
public:
	float angle;
	//float bufferBytes;
};
struct SpotLightExport {
	DirectX::XMFLOAT4 ambientColor;
	DirectX::XMFLOAT4 diffuseColor;
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 direction;
	float angle;
	//float bufferBytes;
};

struct Shadow {
	//virtual ~Shadow() {};
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDepthStencilView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowShaderResourceView;
	DirectX::XMFLOAT4X4 shadowViewMatrix;
	DirectX::XMFLOAT4X4 shadowProjectionMatrix;	// make this a vector and use [0] for non cascade calculatons
};

static const unsigned int MAX_NUM_SHADOW_CASCADES = 5;	// MAX_NUM_SHADOW_CASCADES must have equal value to variable of the same name in shader

struct CascadedShadow : public Shadow {
	CascadedShadow(const Shadow& pShadow)
	{
		shadowDepthStencilView = pShadow.shadowDepthStencilView;
		shadowViewMatrix = pShadow.shadowViewMatrix;	// loop through matrices to copy?
	}
	//int mNumCascades;
	DirectX::XMFLOAT4X4 mCascadeProjections[3];
	//DirectX::XMFLOAT4 mCascadePartitionDepths[1];
	float mCascadePartitionDepths[2];
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mCascadeTexture;
};

class LightComponent : public Component
{
public:
	LightComponent();
	LightComponent(std::shared_ptr<Light> _light);
	virtual ~LightComponent();

	Light* GetGenericLight();
	template<class T>
	T* GetLight()
	{
		return std::dynamic_pointer_cast<T>(light);
	};

	std::shared_ptr<Shadow>& GetShadow();

	template<class T>
	T& GetShadowByType() const
	{
		const std::type_info& searchType = typeid(T);
		if (shadow && typeid(*shadow.get()) == searchType)
		{
			return static_cast<T&>(*shadow.get());	// try static_cast and removing virtual, if possible then can do same for lights and skip light export
		}
	};

private:
	std::shared_ptr<Light> light;
	std::shared_ptr<Shadow> shadow;
};
