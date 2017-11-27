#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <wrl.h>

#include "..\Shader.h"

struct QuadVertex
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT2 uv;
};

class QuadRenderer
{
public:
	//QuadRenderer();
	QuadRenderer(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context);
	~QuadRenderer();

	void SetVertexShader();
	void Draw();
	void Draw(float width, float height);

private:
	// References
	ID3D11DeviceContext* mp_context;

	// Shaders
	std::unique_ptr<VertexShader> m_upvsQuadVertex;

	// Resources
	QuadVertex vertices[6];
	UINT indices[6];
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
};

