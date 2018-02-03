/*	=====================================================================================================
		Pixel Shader
		Particle Rendering
	=====================================================================================================	*/

SamplerState sampler_bilinear	: register(s0);
Texture2D particle_texture		: register(t0);

struct VsOut
{
	float4 position		: SV_POSITION;
	float2 tex_coord	: TEXCOORD;
	float4 color		: COLOR;
};

float4 main(VsOut input) : SV_TARGET
{
	return input.color * particle_texture.Sample(sampler_bilinear, input.tex_coord);
}