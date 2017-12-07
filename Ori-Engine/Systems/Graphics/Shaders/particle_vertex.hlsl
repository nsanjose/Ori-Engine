/*	=====================================================================================================
		Vertex Shader
		Particle Rendering
	=====================================================================================================	*/

cbuffer BufferParameters : register(b0)
{
	matrix world_view_projection_matrix;
};

struct VsIn
{
	float4 position		: POSITION;
	//float2 tex_coord	: TEXCOORD;
	float4 color		: COLOR;
};

struct VsOut
{
	float4 position		: SV_POSITION;
	//float2 tex_coord	: TEXCOORD;
	float4 color		: COLOR;
};

VsOut main(VsIn input)
{	
	VsOut output;
	output.position = mul(input.position, world_view_projection_matrix);
	//output.tex_coord = input.tex_coord;
	output.color = input.color;
	return output;
}