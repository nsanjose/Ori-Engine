#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <vector>
#include <wrl.h>

#include "..\Tools\QuadRenderer.h"
#include "..\Shader.h"

class PostProcessor
{
public:
	PostProcessor(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, const unsigned int& pr_width, const unsigned int& pr_height, QuadRenderer& pr_quad_renderer);
	~PostProcessor();

	const D3D11_TEXTURE2D_DESC* GetFrameBufferDesc() const;
	ID3D11RenderTargetView* GetFrameBufferRtv() const;
	ID3D11ShaderResourceView* GetFrameBufferSrv() const;
	//ID3D11RenderTargetView* GetFrameBuffer2Rtv() const;
	//ID3D11ShaderResourceView* GetFrameBuffer2Srv() const;

	void InitializeFrameBuffers(float pWidth, float pHeight);

/*	=====================================================================================================
		Blur

	-	This effect causes pixel's colors to bleed into their neighbors.
	-	The distribution array passed to the shader specifies the axis of the one dimensional blur.
	=====================================================================================================	*/
	void InitializeBlur();
	//void Blur(ID3D11ShaderResourceView* pSourceTexture, std::vector<float>& , ID3D11RenderTargetView* pDestinationTexture);

/*	=====================================================================================================
		Bloom

	-	This effect causes bright areas to bleed over other pixels.
	-	The effect is generated but not applied until Tone Mapping (Plus)

	1.	From a source texture, copy only pixels above a brightness threshold into an extraction texture.
	2.	Apply a horizontal blur, then a vertical blur to the extraction texture.
	3.	Combine the source with the blurred extraction texture.

	Improve: Blurring downsamples smaller than the source texture gives practically identical results,
			with significant optimization.
			2.	Downsample the extraction texture four times into half by half textures.
			3.	Starting from the smallest, blur each downsample then upsample and add it to the next bigger downsample.
			4.	Upsample and add the final blurred downsample with the original input texture into the destination texture.

	Improve: Use off center offsets for linear sampling count optimization?
	Improve: Look into compute shader for computation and resource optimization?
	=====================================================================================================	*/ 
	void InitializeBloom();
	void Bloom(ID3D11ShaderResourceView* psrvSource);

/*	=====================================================================================================
		Tone Map (Plus)

	-	This effect balances brightness to maximize detail in the brightest and darkest areas, or apply
		a specified balance curve.
	-	The (Plus) in Tone Map (Plus) refers to the shader also applying the final step of other effects
		that are order sensitive.
	=====================================================================================================	*/
	void InitializeToneMap();
	void GenerateAverageLuminance(ID3D11ShaderResourceView * pSourceTextureSrv, float pDeltaTime);
	void ToneMap(ID3D11ShaderResourceView * pSourceTextureSrv, ID3D11RenderTargetView * pSourceTextureRtv);

private:
	// References
	ID3D11Device* mp_device;
	ID3D11DeviceContext* mp_context;
	const unsigned int& mr_width;
	const unsigned int& mr_height;
	QuadRenderer& mr_quad_renderer;

	// Samplers
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_sampler_point;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_sampler_bilinear;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> mcp_sampler_trilinear;

	// Frame Buffers Resources
	D3D11_TEXTURE2D_DESC m_frame_buffer_desc;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mcp_frame_buffer_rtv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_frame_buffer_srv;

	// Blur Resources
	bool m_is_blur_initialized = false;
	std::unique_ptr<PixelShader> mup_blur_pixel_shader;
	float m_blur_standard_deviation;
	static const int m_BLUR_SAMPLE_COUNT = 15;	// m_BLUR_SAMPLE_COUNT must be equal to SAMPLE_COUNT in Gaussian Blur pixel shader, and an odd number.
	float m_blur_sample_offsets[m_BLUR_SAMPLE_COUNT], m_blur_sample_weights[m_BLUR_SAMPLE_COUNT];
	DirectX::XMFLOAT4 m_blur_sample_offsets_and_weights[m_BLUR_SAMPLE_COUNT];	// Offsets: X,Y; Weights: Z; Empty: W
	void InitializeBlurDistribution(float pSampleOffsets[m_BLUR_SAMPLE_COUNT], float pSampleWeights[m_BLUR_SAMPLE_COUNT]);
	void InitializeBlurDistributionForShader(float * sampleOffsets, float * sampleWeights, DirectX::XMFLOAT2 axis, float textureWidth, float textureHeight);

	// Bloom Resources
	bool m_is_bloom_initialized = false;
	std::unique_ptr<PixelShader> mup_bloom_extract_pixel_shader;
	std::unique_ptr<PixelShader> mup_bloom_combine_pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mcp_bloom_extract_rtv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_bloom_extract_srv;
	static const int m_BLOOM_DOWNSAMPLE_COUNT = 4;
	std::vector<D3D11_TEXTURE2D_DESC> m_downsample_descs;
	std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> mcp_bloom_vertical_blur_downsample_rtvs;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> mcp_bloom_vertical_blur_downsample_srvs;
	std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> mcp_bloom_horizontal_blur_downsample_rtvs;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> mcp_bloom_horizontal_blur_downsample_srvs;
	Microsoft::WRL::ComPtr<ID3D11BlendState> mcp_bloom_combine_blend_state;

	// Tone Map Resources
	std::unique_ptr<PixelShader> mup_luminance_buffer_pixel_shader;
	std::unique_ptr<PixelShader> mup_luminance_adapt_pixel_shader;
	std::unique_ptr<PixelShader> mup_tone_map_pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mcp_current_luminance_rtv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_current_luminance_srv;
	float m_adapted_luminance_texture_max_mip_level;
	bool m_current_adapted_luminance_resource_i = 0;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mcp_adapted_luminance_rtvs[2];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_adapted_luminance_first_mip_srvs[2];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_adapted_luminance_last_mip_srvs[2];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mcp_adapted_luminance_all_mips_srvs[2];
};

