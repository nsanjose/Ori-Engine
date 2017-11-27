/*	=====================================================================================================
		Pixel Shader
		SkyBox Rendering
	=====================================================================================================	*/

SamplerState sampler_ansio	: register(s0);
TextureCube skybox			: register(t0);

struct VsOut
{
	float4 position		: SV_POSITION;
	float3 tex_coord	: TEXCOORD;
};

float4 main(VsOut input) : SV_TARGET
{
	return skybox.Sample(sampler_ansio, input.tex_coord);
}