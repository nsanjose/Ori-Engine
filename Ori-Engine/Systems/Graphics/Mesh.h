#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

#include <vector>
#include <wrl.h>

struct Vertex {
	Vertex(DirectX::XMFLOAT3 a, DirectX::XMFLOAT2 b, DirectX::XMFLOAT3 c, DirectX::XMFLOAT3 d)
	{
		position = a;
		tex_coord = b;
		normal = c;
		tangent = d;
	}
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 tex_coord;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
};

class Mesh
{
public:
	Mesh(ID3D11Device* pp_device, const char* pp_filepath);
	~Mesh();

	ID3D11Buffer* GetVertexBuffer();
	ID3D11Buffer* GetIndexBuffer();
	size_t GetVertexCount();
	size_t GetIndexCount();

private:
	bool LoadMesh(const char* filepath);
	void CreateBuffers(ID3D11Device * pp_device, Vertex * pp_vertices, size_t p_vertex_count, UINT * pp_indices, size_t p_index_count);

	std::vector<Vertex> m_vertices;
	std::vector<UINT> m_indices;
	size_t m_vertex_count;
	size_t m_index_count;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mcp_vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mcp_index_buffer;
};