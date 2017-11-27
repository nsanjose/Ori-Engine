/*	=====================================================================================================
		Vertex Shader
		Quad
	=====================================================================================================	*/

struct VsIn
{
	float4 position		: POSITION;
	float2 uv			: TEXCOORD;
};

struct QuadVsOut
{
	float4 position				: SV_POSITION;
	float2 uv					: TEXCOORD0;
};

QuadVsOut main(VsIn input)
{
	QuadVsOut output;
	output.position = input.position;
	output.uv = input.uv;
	return output;
}