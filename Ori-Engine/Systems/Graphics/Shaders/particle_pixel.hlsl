/*	=====================================================================================================
		Pixel Shader
		Particle Rendering
	=====================================================================================================	*/

//SamplerState sampler_linear		: register(s0);
//Texture2D particle_texture		: register(t0);

struct VsOut
{
	float4 position		: SV_POSITION;
	//float2 tex_coord	: TEXCOORD;
	float4 color		: COLOR;
};

float4 main(VsOut input) : SV_TARGET
{
	//float4 texture_color = particle_texture.Sample(sampler_linear, input.tex_coord);
	return input.color;// *texture_color;
}