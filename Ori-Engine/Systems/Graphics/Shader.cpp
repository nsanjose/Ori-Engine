#include "Shader.h"

AbstractShader::AbstractShader(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context)
	: mp_device(pp_device), mp_context(pp_context)
{
}

AbstractShader::~AbstractShader()
{
}

bool AbstractShader::InitializeShaderFromFile(LPCWSTR shaderFile)
{
	HRESULT hr = S_OK;
	hr = D3DReadFileToBlob(shaderFile, mcp_shader_blob.GetAddressOf());
	if (FAILED(hr)) { return false; }

	// Create derived shader
	m_is_shader_initialized = CreateDerivedShader(mcp_shader_blob.Get());
	if (!m_is_shader_initialized) { return false; }

	// Create reflection variables to simplify interaction
	ID3D11ShaderReflection* shader_reflection;
	hr = D3DReflect(mcp_shader_blob->GetBufferPointer(), mcp_shader_blob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&shader_reflection);
	if (FAILED(hr)) { return false; }

	D3D11_SHADER_DESC shader_desc;
	hr = shader_reflection->GetDesc(&shader_desc);
	if (FAILED(hr)) { return false; }

	// Create shader input variable reflection collections

	// Bound input reflections (texture srvs, samplers)
	unsigned int bound_resouce_count = shader_desc.BoundResources;
	for (unsigned int bound_resource_i = 0; bound_resource_i < bound_resouce_count; bound_resource_i++)
	{
		D3D11_SHADER_INPUT_BIND_DESC bound_resource_desc;
		shader_reflection->GetResourceBindingDesc(bound_resource_i, &bound_resource_desc);
		switch (bound_resource_desc.Type)
		{
			case D3D_SIT_SAMPLER:
			{
				m_sampler_reflections[bound_resource_desc.Name].m_register_index = bound_resource_desc.BindPoint;
			}
			break;

			case D3D_SIT_TEXTURE:
			{
				m_srv_reflections[bound_resource_desc.Name].m_register_index = bound_resource_desc.BindPoint;
			}
			break;
		}
	}

	// Constant buffer input reflections
	for (unsigned int buffer_i = 0; buffer_i < shader_desc.ConstantBuffers; buffer_i++)
	{

		ID3D11ShaderReflectionConstantBuffer* shader_reflection_constant_buffer = shader_reflection->GetConstantBufferByIndex(buffer_i);
		D3D11_SHADER_BUFFER_DESC buffer_desc;
		shader_reflection_constant_buffer->GetDesc(&buffer_desc);
		D3D11_SHADER_INPUT_BIND_DESC bind_desc;
		shader_reflection->GetResourceBindingDescByName(buffer_desc.Name, &bind_desc);

		m_constant_buffer_reflections.push_back(ConstantBufferReflection());
		ConstantBufferReflection& constant_buffer_reflection = m_constant_buffer_reflections.back();

		D3D11_BUFFER_DESC temp_constant_buffer_desc		= {};
		temp_constant_buffer_desc.Usage					= D3D11_USAGE_DEFAULT;
		temp_constant_buffer_desc.ByteWidth				= buffer_desc.Size;
		temp_constant_buffer_desc.BindFlags				= D3D11_BIND_CONSTANT_BUFFER;
		temp_constant_buffer_desc.CPUAccessFlags		= 0;
		temp_constant_buffer_desc.MiscFlags				= 0;
		temp_constant_buffer_desc.StructureByteStride	= 0;
		mp_device->CreateBuffer(&temp_constant_buffer_desc, 0, constant_buffer_reflection.mcp_constant_buffer.GetAddressOf());

		constant_buffer_reflection.m_register_index = bind_desc.BindPoint;
		constant_buffer_reflection.m_temp_constant_buffer = new unsigned char[buffer_desc.Size];
		ZeroMemory(constant_buffer_reflection.m_temp_constant_buffer, buffer_desc.Size);

		// Constant buffer variable reflections
		for (unsigned int variable_i = 0; variable_i < buffer_desc.Variables; variable_i++)
		{
			ID3D11ShaderReflectionVariable* shader_reflection_variable = shader_reflection_constant_buffer->GetVariableByIndex(variable_i);
			D3D11_SHADER_VARIABLE_DESC variable_desc;
			shader_reflection_variable->GetDesc(&variable_desc);
			std::string variable_name(variable_desc.Name);

			ConstantBufferVariableReflection& variable_reflection = m_constant_buffer_variable_reflections[variable_name];
			variable_reflection.m_constant_buffer_index = buffer_i;
			variable_reflection.m_byte_offset = variable_desc.StartOffset;
			variable_reflection.m_size = variable_desc.Size;
		}
	}

	shader_reflection->Release();
	return true;
}

void AbstractShader::SetShader()
{
	if (!m_is_shader_initialized) return;

	SetDerivedShader();
}

void AbstractShader::UpdateAllConstantBuffers()
{
	if (!m_is_shader_initialized) return;

	for (unsigned int i = 0; i < m_constant_buffer_reflections.size(); i++)
	{
		mp_context->UpdateSubresource(
			m_constant_buffer_reflections[i].mcp_constant_buffer.Get(), 0, 0,
			m_constant_buffer_reflections[i].m_temp_constant_buffer, 0, 0);
	}
}

const ConstantBufferVariableReflection* AbstractShader::FindConstantBufferVariable(std::string p_name)
{
	std::unordered_map<std::string, ConstantBufferVariableReflection>::const_iterator find_result = m_constant_buffer_variable_reflections.find(p_name);
	if (find_result == m_constant_buffer_variable_reflections.end()) return nullptr;

	const ConstantBufferVariableReflection* variable_reflection = &(find_result->second);
	return variable_reflection;
}

bool AbstractShader::SetConstantBufferVariable(std::string name, const void* data, unsigned int size)
{
	const ConstantBufferVariableReflection* variable_reflection = FindConstantBufferVariable(name);
	// Verify it was found and correct byte size, accept reference to data instead of pointer and use sizeof() here?
	if (variable_reflection == nullptr) return false;
	if (size != variable_reflection->m_size) return false;

	// Set in temporary buffer until calling UpdateAllConstantBuffers()
	memcpy(
		m_constant_buffer_reflections[variable_reflection->m_constant_buffer_index].m_temp_constant_buffer + variable_reflection->m_byte_offset,
		data,
		variable_reflection->m_size);
	return true;
}

const ShaderResourceViewReflection* AbstractShader::FindShaderResourceViewReflection(std::string name)
{
	std::unordered_map<std::string, ShaderResourceViewReflection>::iterator result = m_srv_reflections.find(name);
	if (result == m_srv_reflections.end()) return nullptr;

	const ShaderResourceViewReflection* srv_reflection = &(result->second);
	return srv_reflection;
}

const SamplerReflection* AbstractShader::FindSamplerReflection(std::string name)
{
	std::unordered_map<std::string, SamplerReflection>::iterator result = m_sampler_reflections.find(name);
	if (result == m_sampler_reflections.end()) return nullptr;

	const SamplerReflection* sampler_reflection = &(result->second);
	return sampler_reflection;
}

/*	=====================================================================================================
		Derived Vertex Shader
	=====================================================================================================	*/
VertexShader::VertexShader(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context)
	: AbstractShader(pp_device, pp_context)
{
}

VertexShader::~VertexShader()
{
}

bool VertexShader::CreateDerivedShader(ID3DBlob* pp_shader_blob)
{
	HRESULT hr = mp_device->CreateVertexShader(pp_shader_blob->GetBufferPointer(), pp_shader_blob->GetBufferSize(), 0, mcp_shader.GetAddressOf());
	if (FAILED(hr)) return false;

	// Automatic input layout created from reflection based upon
	// https://takinginitiative.wordpress.com/2011/12/11/directx-1011-basic-shader-reflection-automatic-input-layout-creation/

	ID3D11ShaderReflection* shader_reflection;
	hr = D3DReflect(pp_shader_blob->GetBufferPointer(), pp_shader_blob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)(&shader_reflection));
	if (FAILED(hr)) return false;

	// Get shader info
	D3D11_SHADER_DESC shader_desc;
	shader_reflection->GetDesc(&shader_desc);

	// Read input layout description from shader info
	std::vector<D3D11_INPUT_ELEMENT_DESC> input_layout_desc;
	for (unsigned int i = 0; i< shader_desc.InputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC input_parameter_desc;
		shader_reflection->GetInputParameterDesc(i, &input_parameter_desc);

		// Fill out input element desc
		D3D11_INPUT_ELEMENT_DESC element_desc;
		element_desc.SemanticName			= input_parameter_desc.SemanticName;
		element_desc.SemanticIndex			= input_parameter_desc.SemanticIndex;
		element_desc.InputSlot				= 0;
		element_desc.AlignedByteOffset		= D3D11_APPEND_ALIGNED_ELEMENT;
		element_desc.InputSlotClass			= D3D11_INPUT_PER_VERTEX_DATA;
		element_desc.InstanceDataStepRate	= 0;

		// Use (vertex) buffer slot 1 for instance buffer elements
		std::string instance_semantic_prefix = "INSTANCE_";
		std::string input_semantic = input_parameter_desc.SemanticName;
		if (input_semantic.compare(0, instance_semantic_prefix.size(), instance_semantic_prefix) == 0)
		{
			element_desc.InputSlot				= 1;
			element_desc.InputSlotClass			= D3D11_INPUT_PER_INSTANCE_DATA;
			element_desc.InstanceDataStepRate	= 1;
		}

		// Determine DXGI format
		if (input_parameter_desc.Mask == 1)
		{
			if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element_desc.Format = DXGI_FORMAT_R32_UINT;
			else if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element_desc.Format = DXGI_FORMAT_R32_SINT;
			else if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element_desc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (input_parameter_desc.Mask <= 3)
		{
			if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element_desc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element_desc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (input_parameter_desc.Mask <= 7)
		{
			if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element_desc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element_desc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (input_parameter_desc.Mask <= 15)
		{
			if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element_desc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (input_parameter_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		// Save element desc
		input_layout_desc.push_back(element_desc);
	}

	// Try to create Input Layout
	hr = mp_device->CreateInputLayout(
		&input_layout_desc[0],
		input_layout_desc.size(),
		mcp_shader_blob->GetBufferPointer(),
		mcp_shader_blob->GetBufferSize(),
		mcp_input_layout.GetAddressOf());
	if (FAILED(hr)) return false;

	shader_reflection->Release();
	return true;
}

void VertexShader::SetDerivedShader()
{
	if (!m_is_shader_initialized) return;

	mp_context->VSSetShader(mcp_shader.Get(), 0, 0);

	// Also sets input layout
	mp_context->IASetInputLayout(mcp_input_layout.Get());

	// Also sets constant buffers
	for (unsigned int i = 0; i < m_constant_buffer_reflections.size(); i++)
	{
		mp_context->VSSetConstantBuffers(m_constant_buffer_reflections[i].m_register_index, 1, m_constant_buffer_reflections[i].mcp_constant_buffer.GetAddressOf());
	}
}

bool VertexShader::SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv)
{
	const ShaderResourceViewReflection* srv_reflection = FindShaderResourceViewReflection(name);
	if (srv_reflection == nullptr) return false;

	mp_context->VSSetShaderResources(srv_reflection->m_register_index, 1, &srv);
	return true;
}

bool VertexShader::SetSamplerState(std::string name, ID3D11SamplerState* samplerState)
{
	const SamplerReflection* sampler_reflection = FindSamplerReflection(name);
	if (sampler_reflection == nullptr) return false;

	mp_context->VSSetSamplers(sampler_reflection->m_register_index, 1, &samplerState);
	return true;
}

/*	=====================================================================================================
		Derived Pixel Shader
	=====================================================================================================	*/
PixelShader::PixelShader(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context)
	: AbstractShader(pp_device, pp_context)
{
}

PixelShader::~PixelShader()
{
}

bool PixelShader::CreateDerivedShader(ID3DBlob* pp_shader_blob)
{
	HRESULT hr = mp_device->CreatePixelShader(pp_shader_blob->GetBufferPointer(), pp_shader_blob->GetBufferSize(), 0, mcp_shader.GetAddressOf());
	if (FAILED(hr)) return false;
	return true;
}

void PixelShader::SetDerivedShader()
{
	if (!m_is_shader_initialized) return;

	mp_context->PSSetShader(mcp_shader.Get(), 0, 0);

	// Also set constant buffers
	for (unsigned int i = 0; i < m_constant_buffer_reflections.size(); i++)
	{
		mp_context->PSSetConstantBuffers(m_constant_buffer_reflections[i].m_register_index, 1, m_constant_buffer_reflections[i].mcp_constant_buffer.GetAddressOf());
	}
}

bool PixelShader::SetShaderResourceView(std::string p_name, ID3D11ShaderResourceView* pp_srv)
{
	const ShaderResourceViewReflection* srv_reflection = FindShaderResourceViewReflection(p_name);
	if (srv_reflection == nullptr) return false;

	mp_context->PSSetShaderResources(srv_reflection->m_register_index, 1, &pp_srv);
	return true;
}

bool PixelShader::SetSamplerState(std::string pp_name, ID3D11SamplerState* pp_sampler)
{
	const SamplerReflection* sampler_reflection = FindSamplerReflection(pp_name);
	if (sampler_reflection == nullptr) return false;

	mp_context->PSSetSamplers(sampler_reflection->m_register_index, 1, &pp_sampler);
	return true;
}