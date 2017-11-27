/*	=====================================================================================================
		Vertex Shader
		Shadow Rendering

	-	Only relevant factor for shadowing is position.
	=====================================================================================================	*/

cbuffer BufferParameters : register(b0)
{
	matrix world_view_projection_matrix;
};

struct VsIn
{
	float3 position		: POSITION;
	float2 tex_coord	: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
};

struct ShadowVsOut
{
	float4 position		: SV_POSITION;
};

ShadowVsOut main(VsIn input)
{
	ShadowVsOut output;
	output.position = mul(float4(input.position, 1.0f), world_view_projection_matrix);
	return output;
}