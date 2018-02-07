#pragma once

#pragma comment(lib, "d3dcompiler.lib")

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include <d3dcompiler.h>
#include <d3d11.h>
#include <DirectXMath.h>

/*	=====================================================================================================
		Shader Input Type Reflections
	=====================================================================================================	*/
struct ConstantBufferVariableReflection
{
	unsigned int m_constant_buffer_index;
	unsigned int m_byte_offset;
	unsigned int m_size;
};

struct ConstantBufferReflection
{
	~ConstantBufferReflection() { delete[] m_temp_constant_buffer; }
	unsigned char* m_temp_constant_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mcp_constant_buffer;
	unsigned int m_register_index;
	D3D_CBUFFER_TYPE m_type;
};

struct ShaderResourceViewReflection
{
	unsigned int m_register_index;
};

struct SamplerReflection
{
	unsigned int m_register_index;
};

/*	=====================================================================================================
		Base Shader
	=====================================================================================================	*/
class AbstractShader
{
public:
	AbstractShader(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context);
	virtual ~AbstractShader();

	bool InitializeShaderFromFile(LPCWSTR p_filepath);

	void SetShader();
	void UpdateAllConstantBuffers();

	bool SetInterfaceToClass(LPCSTR p_interface_name, LPCSTR p_class_name);
	bool SetConstantBufferVariable(std::string p_name, const void* pp_data, unsigned int p_byte_size);
	bool SetConstantBufferInt(std::string p_name, int p_data)								{ return SetConstantBufferVariable(p_name, &p_data, sizeof(int)); }
	bool SetConstantBufferFloat(std::string p_name, float p_data)							{ return SetConstantBufferVariable(p_name, &p_data, sizeof(float)); }
	bool SetConstantBufferFloat2(std::string p_name, const DirectX::XMFLOAT2 p_data)		{ return SetConstantBufferVariable(p_name, &p_data, sizeof(float) * 2); }
	bool SetConstantBufferFloat3(std::string p_name, const DirectX::XMFLOAT3 p_data)		{ return SetConstantBufferVariable(p_name, &p_data, sizeof(float) * 3); }
	bool SetConstantBufferFloat4(std::string p_name, const DirectX::XMFLOAT4 p_data)		{ return SetConstantBufferVariable(p_name, &p_data, sizeof(float) * 4); }
	bool SetConstantBufferMatrix4x4(std::string p_name, const DirectX::XMFLOAT4X4 p_data)	{ return SetConstantBufferVariable(p_name, &p_data, sizeof(float) * 16); }

	// Depends on derived shader type
	virtual bool SetSamplerState(std::string p_name, ID3D11SamplerState* pp_sampler) = 0;
	virtual bool SetShaderResourceView(std::string p_name, ID3D11ShaderResourceView* pp_srv) = 0;

	// Used by derived setters to find appropriate register
	const ShaderResourceViewReflection* FindShaderResourceViewReflection(std::string p_name);
	const SamplerReflection* FindSamplerReflection(std::string p_name);

protected:
	ID3D11Device* mp_device;
	ID3D11DeviceContext* mp_context;

	// Initialization
	bool m_is_shader_initialized;
	Microsoft::WRL::ComPtr<ID3DBlob> mcp_shader_blob;
	Microsoft::WRL::ComPtr<ID3D11ShaderReflection> mcp_shader_reflection;

	// Resources
	std::vector<ConstantBufferReflection> m_constant_buffer_reflections;
	std::unordered_map<std::string, ConstantBufferVariableReflection> m_constant_buffer_variable_reflections;
	std::unordered_map<std::string, SamplerReflection> m_sampler_reflections;
	std::unordered_map<std::string, ShaderResourceViewReflection> m_srv_reflections;

	// Depends on derived shader type
	virtual bool CreateDerivedShader(ID3DBlob* pp_shader_blob, ID3D11ClassLinkage* pp_class_linkage) = 0;
	virtual void SetDerivedShader() = 0;

	// Used by setter to find appropriate memory destination
	const ConstantBufferVariableReflection* FindConstantBufferVariable(std::string p_name);

	Microsoft::WRL::ComPtr<ID3D11ClassLinkage> mcp_class_linkage;
	UINT m_num_interfaces;
	ID3D11ClassInstance** mp_interface_targets;
	std::unordered_map<std::string, ID3D11ClassInstance*> mp_class_instances;

	void RecursiveLayoutReflectionVariableMembers(UINT p_cbuffer_i, std::vector<std::string*>& p_name_hierarchy, UINT p_offset_hierarchy, ID3D11ShaderReflectionType* pp_var_type);
	UINT GetSizeOfShaderVariableType(ID3D11ShaderReflectionType* pp_var_type);
};

/*	=====================================================================================================
		Derived Vertex Shader
	=====================================================================================================	*/
class VertexShader : public AbstractShader
{
public:
	VertexShader(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context);
	~VertexShader();

	bool SetShaderResourceView(std::string p_name, ID3D11ShaderResourceView* pp_srv);
	bool SetSamplerState(std::string p_name, ID3D11SamplerState* pp_sampler);

protected:
	Microsoft::WRL::ComPtr<ID3D11InputLayout> mcp_input_layout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> mcp_shader;
	bool CreateDerivedShader(ID3DBlob* pp_shader_blob, ID3D11ClassLinkage* pp_class_linkage);
	void SetDerivedShader();
};

/*	=====================================================================================================
		Derived Pixel Shader
	=====================================================================================================	*/
class PixelShader : public AbstractShader
{
public:
	PixelShader(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context);
	~PixelShader();

	bool SetShaderResourceView(std::string p_name, ID3D11ShaderResourceView* pp_srv);
	bool SetSamplerState(std::string p_name, ID3D11SamplerState* pp_sampler);

protected:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> mcp_shader;
	bool CreateDerivedShader(ID3DBlob* pp_shader_blob, ID3D11ClassLinkage* pp_class_linkage);
	void SetDerivedShader();
};