/*	=====================================================================================================
		Vertex Shader
		SkyBox Rendering
	=====================================================================================================	*/

cbuffer BufferParameters : register(b0)
{
	matrix view_matrix;
	matrix projection_matrix;
};

struct VsIn
{
	float3 position		: POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
};

struct VsOut
{
	float4 position		: SV_POSITION;
	float3 tex_coord	: TEXCOORD;
};

VsOut main(VsIn input)
{
	// Center skybox on camera
	matrix no_translation_view_matrix = view_matrix;
	no_translation_view_matrix._41 = 0;
	no_translation_view_matrix._42 = 0;
	no_translation_view_matrix._43 = 0;
	matrix view_projection_matrix = mul(no_translation_view_matrix, projection_matrix);
	
	VsOut output;
	output.position = mul(float4(input.position, 1.0f), view_projection_matrix);
	// Max depth
	output.position.z = output.position.w;
	output.tex_coord = input.position;
	return output;
}