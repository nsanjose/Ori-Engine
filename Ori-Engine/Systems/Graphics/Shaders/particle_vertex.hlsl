/*	=====================================================================================================
		Vertex Shader
		Particle Rendering
	=====================================================================================================	*/
#include "common_particle.hlsl"

cbuffer BufferParameters : register(b0)
{
	matrix view_matrix;
	matrix projection_matrix;
};

struct VsIn
{
	float4 position_os	: POSITION;
	float4 color		: INSTANCE_COLOR;
	matrix world_matrix	: INSTANCE_WORLD;
	//float2 tex_coord	: TEXCOORD;
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

	matrix temp_world_view_matrix = mul(input.world_matrix, view_matrix);
	Billboard_ApproximateSpherical(temp_world_view_matrix);

	output.position = mul(input.position_os, mul(temp_world_view_matrix, projection_matrix));

	//output.tex_coord = input.tex_coord;
	output.color = input.color;

	return output;
}