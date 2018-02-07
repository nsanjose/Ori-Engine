/*	=====================================================================================================
		Pixel Shader
		Deferred Shading
		Accumulate Shadows into GBuffer
	=====================================================================================================	*/

static const uint MAX_CASCADES = 5;

cbuffer ConstantBuffer
{
	// CSM
	float4x4 shadow_view_matrix;
	float4x4 cascade_projections[MAX_CASCADES];
	uint num_cascades;

	// PCF
	float shadow_map_width;
	float shadow_map_height;
}

SamplerComparisonState shadow_sampler	: register(s0);
Texture2DArray cascaded_shadow_map		: register(t0);

struct VsOut
{
	float4 position_ps	: SV_POSITION;
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
	// unpack cascade far clips
	float cascade_far_clips[MAX_CASCADES - 1];
	[unroll] for (uint i = 0; i < MAX_CASCADES - 1; i++)
	{
		cascade_far_clips[i] = cascade_projections[i]._14;
	}

	// cascade selection
	// use atlas texture to avoid branching?
	float depth = input.position_vs.z;
	uint cascade_i = 0;
	for (uint i = 0; i < num_cascades - 1; i++)
	{
		if (depth > cascade_far_clips[i]) { cascade_i++; }
	}

	// remove packed far clip from projection
	// maybe not worth avoiding transfer because it requires copying to an editable matrix?
	float4x4 cascade_projection = cascade_projections[cascade_i];
	cascade_projection._14 = 0;

	// calculating shadow map uv
	float4 position_shadow_clip_space	= mul(mul(input.position_ws, shadow_view_matrix), cascade_projection);
	float2 shadow_tex_coord				= position_shadow_clip_space.xy / position_shadow_clip_space.w * 0.5f + 0.5f;
	shadow_tex_coord.y					= 1 - shadow_tex_coord.y;

	float depth_from_light = position_shadow_clip_space.z / position_shadow_clip_space.w;
	
	// PCF
	float shadow = 0;
	for (float x_offset = -1.5; x_offset <= 1.5; x_offset += 1)
	{
		for (float y_offset = -1.5; y_offset <= 1.5; y_offset += 1)
		{
			float2 offset = float2(x_offset / shadow_map_width, y_offset / shadow_map_height);
			shadow += cascaded_shadow_map.SampleCmp(shadow_sampler, float3(shadow_tex_coord + offset, cascade_i), depth_from_light);
		}
	}
	shadow /= 16;

	PsOut output;
	output.texture_one		= float4(0, 0, 0, shadow);
	output.texture_two		= float4(0, 0, 0, 0);
	output.texture_three	= float4(0, 0, 0, 0);
	return output;
}