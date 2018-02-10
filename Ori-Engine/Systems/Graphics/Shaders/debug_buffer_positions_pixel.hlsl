/*	=====================================================================================================
		Pixel Shader
		Debugging: buffering positions for reconstruction verifcation
	=====================================================================================================	*/

struct VsOut
{
	float4 position		: SV_POSITION;
	float4 position_cs	: POSITION_CS;
	float4 position_vs	: POSITION_VS;
	float4 position_ws	: POSITION_WS;
};

struct PsOut 
{
	float4 texture_one		: SV_TARGET0;
	float4 texture_two		: SV_TARGET1;
	float4 texture_three	: SV_TARGET2;
};

PsOut main(VsOut input)
{
	PsOut output;
	output.texture_one		= input.position_cs;
	output.texture_two		= input.position_vs;
	output.texture_three	= input.position_ws;
	return output;
}