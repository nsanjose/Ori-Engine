/*	=====================================================================================================
		Pixel Shader
		Deferred Shading
		Clear any gbuffer channels using write masks and blend states
	=====================================================================================================	*/

struct QuadVsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
};

struct PsOut 
{
	float4 texture_one		: SV_TARGET0;
	float4 texture_two		: SV_TARGET1;
	float4 texture_three	: SV_TARGET2;
};

PsOut main(QuadVsOut input)
{
	PsOut output;
	output.texture_one		= float4(1, 1, 1, 1);
	output.texture_two		= float4(1, 1, 1, 1);
	output.texture_three	= float4(1, 1, 1, 1);
	return output;
}