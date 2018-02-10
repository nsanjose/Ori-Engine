/*	=====================================================================================================
		Vertex Shader
		Deferred Shading 
		First Pass: Populate GBuffers with data
	=====================================================================================================	*/

cbuffer BufferParameters : register(b0)
{
	float4x4 world_view_matrix;
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
	output.position		= mul(float4(input.position, 1.0f), world_view_projection_matrix);
	output.tex_coord	= input.tex_coord;
	output.normal		= mul(input.normal, world_view_matrix);
	output.tangent		= mul(input.tangent, world_view_matrix);
	return output;
}