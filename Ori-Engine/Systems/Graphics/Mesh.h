#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <wrl.h>

struct Vertex {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
};

class Mesh
{
public:
	Mesh();
	Mesh(ID3D11Device* pp_device, const char* pp_filepath);
	~Mesh();

	ID3D11Buffer* GetVertexBuffer();
	ID3D11Buffer* GetIndexBuffer();
	std::vector<Vertex> GetVertices();
	int GetVertexCount();
	int GetIndexCount();

private:
	std::vector<Vertex> vertices;

	// Buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer> mcp_vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mcp_index_buffer;

	// Counts
	int m_vertex_count;
	int m_index_count;

	void CreateBuffers(Vertex* vertices, int numVerts, unsigned int* indices, int numInds, ID3D11Device* device);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
};