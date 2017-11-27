/*	=====================================================================================================
		Vertex Shader
		Deferred Shading 
		First Pass: Populate GBuffers with data
	=====================================================================================================	*/

cbuffer BufferParameters : register(b0)
{
	float4x4 world_matrix;
	//float4x4 inverse_transpose_world_matrix;
	float4x4 world_view_projection_matrix;
};

struct VsIn
{
	float3 position		: POSITION;
	float2 tex_coord	: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
};

struct VsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
};

VsOut main(VsIn input)
{
	VsOut output;

	output.position =  mul(float4(input.position, 1.0f), world_view_projection_matrix);
	output.tex_coord = input.tex_coord;

	output.normal = mul(input.normal, (float3x3)world_matrix); // if uniform scale
	output.tangent = mul(input.tangent, (float3x3)world_matrix);
	//output.normal = mul(input.normal, (float3x3)inverse_transpose_world_matrix); // if not
	//output.tangent = mul(input.tangent, (float3x3)inverse_transpose_world_matrix);
	
	return output;
}