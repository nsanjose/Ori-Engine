#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX;
using namespace Assimp;

Mesh::Mesh(ID3D11Device* device, const char * objFile)
{
	LoadMesh(objFile);
	CreateBuffers(device, &m_vertices[0], m_vertex_count, &m_indices[0], m_index_count);
}

Mesh::~Mesh()
{
}

ID3D11Buffer* Mesh::GetVertexBuffer()
{
	return mcp_vertex_buffer.Get();
}

ID3D11Buffer* Mesh::GetIndexBuffer()
{
	return mcp_index_buffer.Get();
}

size_t Mesh::GetVertexCount()
{
	return m_vertex_count;
}

size_t Mesh::GetIndexCount()
{
	return m_index_count;
}

bool Mesh::LoadMesh(const char* filepath)
{
	Importer importer;
	const aiScene* scene = importer.ReadFile(filepath,
		aiProcess_ConvertToLeftHanded	|
		aiProcess_CalcTangentSpace		|
		aiProcess_GenSmoothNormals		|
		aiProcess_JoinIdenticalVertices |
		aiProcess_OptimizeMeshes		|
		aiProcess_OptimizeGraph			|
		aiProcess_Triangulate);
	if (!scene) { return false; }
	if (!scene->HasMeshes()) { return false; }

	const aiMesh* mesh = scene->mMeshes[0];

	m_vertex_count = mesh->mNumVertices;
	for (size_t i = 0; i < m_vertex_count; i++)
	{
		m_vertices.emplace_back(
			XMFLOAT3(
				mesh->mVertices[i].x,
				mesh->mVertices[i].y,
				mesh->mVertices[i].z),
			XMFLOAT2(
				mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y),
			XMFLOAT3(
				mesh->mNormals[i].x,
				mesh->mNormals[i].y,
				mesh->mNormals[i].z),
			XMFLOAT3(
				mesh->mTangents[i].x,
				mesh->mTangents[i].y,
				mesh->mTangents[i].z));
	}

	m_index_count = mesh->mNumFaces * 3;
	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		m_indices.emplace_back(mesh->mFaces[i].mIndices[0]);
		m_indices.emplace_back(mesh->mFaces[i].mIndices[1]);
		m_indices.emplace_back(mesh->mFaces[i].mIndices[2]);
	}
}

void Mesh::CreateBuffers(ID3D11Device * pp_device, Vertex * pp_vertices, size_t p_vertex_count, UINT * pp_indices, size_t p_index_count)
{
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage				= D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth			= sizeof(Vertex) * p_vertex_count;
	vbd.BindFlags			= D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags		= 0;
	vbd.MiscFlags			= 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vertex_data;
	vertex_data.pSysMem = pp_vertices;
	pp_device->CreateBuffer(&vbd, &vertex_data, mcp_vertex_buffer.GetAddressOf());

	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage				= D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth			= sizeof(UINT) * p_index_count;
	ibd.BindFlags			= D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags		= 0;
	ibd.MiscFlags			= 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA index_data;
	index_data.pSysMem = pp_indices;
	pp_device->CreateBuffer(&ibd, &index_data, mcp_index_buffer.GetAddressOf());
}