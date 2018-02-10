/*	=====================================================================================================
		Vertex Shader
		Debugging: buffering positions for reconstruction verifcation
	=====================================================================================================	*/

cbuffer BufferParameters : register(b0)
{
	float4x4 world_view_projection_matrix;
	float4x4 world_view_matrix;
	float4x4 world_matrix;
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
	float4 position_cs	: POSITION_CS;
	float4 position_vs	: POSITION_VS;
	float4 position_ws	: POSITION_WS;
};

VsOut main(VsIn input)
{
	VsOut output;
	output.position	= mul(float4(input.position, 1.0f), world_view_projection_matrix);
	output.position_cs = output.position;
	output.position_vs = mul(float4(input.position, 1.0f), world_view_matrix);
	output.position_ws = mul(float4(input.position, 1.0f), world_matrix);
	return output;
}