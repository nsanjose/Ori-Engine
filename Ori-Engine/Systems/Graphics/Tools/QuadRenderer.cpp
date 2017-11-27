#include "QuadRenderer.h"

QuadRenderer::QuadRenderer(ID3D11Device * pp_device, ID3D11DeviceContext* pp_context)
	: mp_context(pp_context)
{

	// Vertex Buffer
	vertices[0].position = { 1.0f, -1.0f, 0.0f, 1.0f };		// Bottom left.
	vertices[0].uv = { 1.0f, 1.0f };
	vertices[1].position = { -1.0f, -1.0f, 0.0f, 1.0f };	// Top left.
	vertices[1].uv = { 0.0f, 1.0f };
	vertices[2].position = { -1.0f, 1.0f, 0.0f, 1.0f };		// Bottom right.
	vertices[2].uv = { 0.0f, 0.0f };
	vertices[3].position = { 1.0f, 1.0f, 0.0f, 1.0f };		// Top right.
	vertices[3].uv = { 1.0f, 0.0f };
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(QuadVertex) * 6;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertices;
	pp_device->CreateBuffer(&vbd, &vertexData, &vertex_buffer);

	// Index Buffer
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 2;
	indices[4] = 3;
	indices[5] = 0;
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(unsigned int) * 6;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	pp_device->CreateBuffer(&ibd, &indexData, &indexBuffer);

	// Vertex Shader
	m_upvsQuadVertex = std::make_unique<VertexShader>(pp_device, mp_context);
	if (!m_upvsQuadVertex->InitializeShaderFromFile(L"x64/Debug/quad_vertex.cso"))
		m_upvsQuadVertex->InitializeShaderFromFile(L"quad_vertex.cso");
}


QuadRenderer::~QuadRenderer()
{
}

void QuadRenderer::SetVertexShader()
{
	m_upvsQuadVertex->SetShader();
}

void QuadRenderer::Draw()
{
	UINT stride = sizeof(QuadVertex);
	UINT offset = 0;
	mp_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
	mp_context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	mp_context->DrawIndexed(6, 0, 0);
}


void QuadRenderer::Draw(float width, float height)
{
	// Save previous viewport
	D3D11_VIEWPORT oldViewports[15];
	UINT oldNumViewports = 6;
	mp_context->RSGetViewports(&oldNumViewports, oldViewports);

	// Set new viewport and draw
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, width, height };
	mp_context->RSSetViewports(1, &vp);

	UINT stride = sizeof(QuadVertex);
	UINT offset = 0;
	mp_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
	mp_context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	mp_context->DrawIndexed(6, 0, 0);

	// Reset viewport
	mp_context->RSSetViewports(1, oldViewports);
}
