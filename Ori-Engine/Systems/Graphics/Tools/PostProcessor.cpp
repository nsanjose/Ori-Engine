#include "PostProcessor.h"

PostProcessor::PostProcessor(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, const unsigned int& pr_width, const unsigned int& pr_height, QuadRenderer& pr_quad_renderer)
	:mp_device(pp_device), mp_context(pp_context), mr_width(pr_width), mr_height(pr_height), mr_quad_renderer(pr_quad_renderer)
{
	// Samplers
	D3D11_SAMPLER_DESC sampler_point_desc = {};
	ZeroMemory(&sampler_point_desc, sizeof(D3D11_SAMPLER_DESC));
	sampler_point_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampler_point_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_point_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_point_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	mp_device->CreateSamplerState(&sampler_point_desc, mcp_sampler_point.GetAddressOf());
	D3D11_SAMPLER_DESC sampler_bilinear_desc = {};
	ZeroMemory(&sampler_bilinear_desc, sizeof(D3D11_SAMPLER_DESC));
	sampler_bilinear_desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampler_bilinear_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_bilinear_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_bilinear_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	mp_device->CreateSamplerState(&sampler_bilinear_desc, mcp_sampler_bilinear.GetAddressOf());
	D3D11_SAMPLER_DESC sampler_linear_desc = {};
	ZeroMemory(&sampler_linear_desc, sizeof(D3D11_SAMPLER_DESC));
	sampler_linear_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_linear_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_linear_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_linear_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	mp_device->CreateSamplerState(&sampler_linear_desc, mcp_sampler_trilinear.GetAddressOf());

	InitializeFrameBuffers((float)mr_width, (float)mr_height);
	InitializeBloom();
	InitializeToneMap();
}

PostProcessor::~PostProcessor()
{
}

const D3D11_TEXTURE2D_DESC* PostProcessor::GetFrameBufferDesc() const
{
	return &m_frame_buffer_desc;
}

ID3D11RenderTargetView* PostProcessor::GetFrameBufferRtv() const
{
	return mcp_frame_buffer_rtv.Get();
}

ID3D11ShaderResourceView* PostProcessor::GetFrameBufferSrv() const
{
	return mcp_frame_buffer_srv.Get();
}

void PostProcessor::InitializeFrameBuffers(float pWidth, float pHeight)
{
	m_frame_buffer_desc.Width				= pWidth;
	m_frame_buffer_desc.Height				= pHeight;
	m_frame_buffer_desc.MipLevels			= 1;
	m_frame_buffer_desc.ArraySize			= 1;
	m_frame_buffer_desc.Format				= DXGI_FORMAT_R16G16B16A16_FLOAT;//DXGI_FORMAT_R32G32B32A32_FLOAT;
	m_frame_buffer_desc.SampleDesc.Count	= 1;
	m_frame_buffer_desc.SampleDesc.Quality	= 0;
	m_frame_buffer_desc.Usage				= D3D11_USAGE_DEFAULT;
	m_frame_buffer_desc.BindFlags			= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	m_frame_buffer_desc.CPUAccessFlags		= 0;
	m_frame_buffer_desc.MiscFlags			= 0;
	ID3D11Texture2D* frameBuffer1;
	ID3D11Texture2D* frameBuffer2;
	mp_device->CreateTexture2D(&m_frame_buffer_desc, nullptr, &frameBuffer1);

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format							= m_frame_buffer_desc.Format;
	rtvDesc.ViewDimension					= D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice				= 0;
	mp_device->CreateRenderTargetView(frameBuffer1, &rtvDesc, mcp_frame_buffer_rtv.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = m_frame_buffer_desc.Format;
	srvDesc.ViewDimension					= D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels				= 1;
	srvDesc.Texture2D.MostDetailedMip		= 0;
	mp_device->CreateShaderResourceView(frameBuffer1, &srvDesc, mcp_frame_buffer_srv.GetAddressOf());

	frameBuffer1->Release();
}

void PostProcessor::InitializeBlur()
{
	m_blur_standard_deviation = 5.0f;	// standard deviation
	mup_blur_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_blur_pixel_shader->InitializeShaderFromFile(L"x64/Debug/post_blur_pixel.cso"))
		mup_blur_pixel_shader->InitializeShaderFromFile(L"post_blur_pixel.cso");

	// Gaussian distribution resources
	InitializeBlurDistribution(m_blur_sample_offsets, m_blur_sample_weights);

	m_is_blur_initialized = true;
}

void PostProcessor::InitializeBloom()
{
	// Check dependencies
	if (!m_is_blur_initialized)
	{ 
		InitializeBlur();
		m_is_blur_initialized = true;
	}

	// Shaders
	mup_bloom_extract_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_bloom_extract_pixel_shader->InitializeShaderFromFile(L"x64/Debug/post_bloom_extract_pixel.cso"))
		mup_bloom_extract_pixel_shader->InitializeShaderFromFile(L"post_bloom_extract_pixel.cso");
	mup_bloom_combine_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_bloom_combine_pixel_shader->InitializeShaderFromFile(L"x64/Debug/post_bloom_combine_pixel.cso"))
		mup_bloom_combine_pixel_shader->InitializeShaderFromFile(L"post_bloom_combine_pixel.cso");

	// Extraction texture resources
	D3D11_TEXTURE2D_DESC bloom_extract_texture_desc = {};
	bloom_extract_texture_desc.Width				= m_frame_buffer_desc.Width;
	bloom_extract_texture_desc.Height				= m_frame_buffer_desc.Height;
	bloom_extract_texture_desc.MipLevels			= 1 + m_BLOOM_DOWNSAMPLE_COUNT;
	bloom_extract_texture_desc.ArraySize			= 1;
	bloom_extract_texture_desc.Format				= m_frame_buffer_desc.Format;
	bloom_extract_texture_desc.SampleDesc.Count		= 1;
	bloom_extract_texture_desc.SampleDesc.Quality	= 0;
	bloom_extract_texture_desc.Usage				= D3D11_USAGE_DEFAULT;
	bloom_extract_texture_desc.BindFlags			= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	bloom_extract_texture_desc.CPUAccessFlags		= 0;
	bloom_extract_texture_desc.MiscFlags			= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	ID3D11Texture2D* bloom_extract_texture;
	mp_device->CreateTexture2D(&bloom_extract_texture_desc, nullptr, &bloom_extract_texture);

	D3D11_RENDER_TARGET_VIEW_DESC bloom_extract_rtv_desc = {};
	bloom_extract_rtv_desc.Format				= bloom_extract_texture_desc.Format;
	bloom_extract_rtv_desc.ViewDimension		= D3D11_RTV_DIMENSION_TEXTURE2D;
	bloom_extract_rtv_desc.Texture2D.MipSlice	= 0;
	mp_device->CreateRenderTargetView(bloom_extract_texture, &bloom_extract_rtv_desc, mcp_bloom_extract_rtv.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC bloom_extract_srv_desc = {};
	bloom_extract_srv_desc.Format						= bloom_extract_texture_desc.Format;
	bloom_extract_srv_desc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
	bloom_extract_srv_desc.Texture2D.MostDetailedMip	= 0;
	bloom_extract_srv_desc.Texture2D.MipLevels			= bloom_extract_texture_desc.MipLevels;
	mp_device->CreateShaderResourceView(bloom_extract_texture, &bloom_extract_srv_desc, mcp_bloom_extract_srv.GetAddressOf());
	//bloom_extract_texture->Release();

	// Vertical blur
	for (UINT downsample_i = 0; downsample_i < m_BLOOM_DOWNSAMPLE_COUNT; downsample_i++)
	{
		D3D11_RENDER_TARGET_VIEW_DESC downsample_rtv_desc = {};
		downsample_rtv_desc.Format				= m_frame_buffer_desc.Format;
		downsample_rtv_desc.ViewDimension		= D3D11_RTV_DIMENSION_TEXTURE2D;
		downsample_rtv_desc.Texture2D.MipSlice	= 1 + downsample_i;
		mcp_bloom_vertical_blur_downsample_rtvs.push_back(Microsoft::WRL::ComPtr<ID3D11RenderTargetView>());
		mp_device->CreateRenderTargetView(bloom_extract_texture, &downsample_rtv_desc, mcp_bloom_vertical_blur_downsample_rtvs.back().GetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC downsample_srv_desc = {};
		downsample_srv_desc.Format						= m_frame_buffer_desc.Format;
		downsample_srv_desc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		downsample_srv_desc.Texture2D.MostDetailedMip	= 1 + downsample_i;
		downsample_srv_desc.Texture2D.MipLevels			= 1;
		mcp_bloom_vertical_blur_downsample_srvs.push_back(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>());
		mp_device->CreateShaderResourceView(bloom_extract_texture, &downsample_srv_desc, mcp_bloom_vertical_blur_downsample_srvs.back().GetAddressOf());
	}
	bloom_extract_texture->Release();

	// Horizontal blur
	UINT downsample_divisor = 1;
	for (UINT downsample_i = 0; downsample_i < m_BLOOM_DOWNSAMPLE_COUNT; downsample_i++)
	{
		// Each downsample's dimensions are halved.
		downsample_divisor *= 2;
		m_downsample_descs.emplace_back();
		m_downsample_descs.back().Width					= m_frame_buffer_desc.Width / downsample_divisor;
		m_downsample_descs.back().Height				= m_frame_buffer_desc.Height / downsample_divisor;
		m_downsample_descs.back().MipLevels				= 1;
		m_downsample_descs.back().ArraySize				= 1;
		m_downsample_descs.back().Format				= m_frame_buffer_desc.Format;
		m_downsample_descs.back().SampleDesc.Count		= 1;
		m_downsample_descs.back().SampleDesc.Quality	= 0;
		m_downsample_descs.back().Usage					= D3D11_USAGE_DEFAULT;
		m_downsample_descs.back().BindFlags				= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		m_downsample_descs.back().CPUAccessFlags		= 0;
		m_downsample_descs.back().MiscFlags				= 0;

		ID3D11Texture2D* downsample_texture;
		mp_device->CreateTexture2D(&m_downsample_descs.back(), nullptr, &downsample_texture);

		D3D11_RENDER_TARGET_VIEW_DESC downsample_rtv_desc = {};
		downsample_rtv_desc.Format				= m_downsample_descs.back().Format;
		downsample_rtv_desc.ViewDimension		= D3D11_RTV_DIMENSION_TEXTURE2D;
		downsample_rtv_desc.Texture2D.MipSlice	= 0;
		mcp_bloom_horizontal_blur_downsample_rtvs.push_back(Microsoft::WRL::ComPtr<ID3D11RenderTargetView>());
		mp_device->CreateRenderTargetView(downsample_texture, &downsample_rtv_desc, mcp_bloom_horizontal_blur_downsample_rtvs.back().GetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC downsample_srv_desc = {};
		downsample_srv_desc.Format						= m_downsample_descs.back().Format;
		downsample_srv_desc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		downsample_srv_desc.Texture2D.MostDetailedMip	= 0;
		downsample_srv_desc.Texture2D.MipLevels			= 1;
		mcp_bloom_horizontal_blur_downsample_srvs.push_back(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>());
		mp_device->CreateShaderResourceView(downsample_texture, &downsample_srv_desc, mcp_bloom_horizontal_blur_downsample_srvs.back().GetAddressOf());

		// Texture references are not needed. This does not unallocate the texture memory.
		downsample_texture->Release();
	}

	// Combine blend state
	D3D11_BLEND_DESC combine_blend_desc = {};
	combine_blend_desc.AlphaToCoverageEnable					= false;
	combine_blend_desc.IndependentBlendEnable					= false;
	combine_blend_desc.RenderTarget[0].BlendEnable				= true;
	combine_blend_desc.RenderTarget[0].SrcBlend					= D3D11_BLEND_ONE;
	combine_blend_desc.RenderTarget[0].DestBlend				= D3D11_BLEND_ONE;
	combine_blend_desc.RenderTarget[0].BlendOp					= D3D11_BLEND_OP_ADD;
	combine_blend_desc.RenderTarget[0].SrcBlendAlpha			= D3D11_BLEND_ZERO;
	combine_blend_desc.RenderTarget[0].DestBlendAlpha			= D3D11_BLEND_ONE;
	combine_blend_desc.RenderTarget[0].BlendOpAlpha				= D3D11_BLEND_OP_ADD;
	combine_blend_desc.RenderTarget[0].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
	mp_device->CreateBlendState(&combine_blend_desc, mcp_bloom_combine_blend_state.GetAddressOf());
}

float GuassianFunction1D(float x, float rho)
{
	// rho is standard deviation
	// rhoSquared is variance
	float rhoSquared2 = rho * rho * 2.0f;

	return expf(-(x * x) / rhoSquared2) / sqrt(DirectX::XM_PI * rhoSquared2);
}

float GuassianFunction2D(float x, float y, float rho)
{
	return expf(-(x * x + y * y) / (2 * rho * rho)) / sqrt(2.0f * DirectX::XM_PI * rho * rho);
}

void PostProcessor::InitializeBlurDistribution(float m_blur_sample_offsets[m_BLUR_SAMPLE_COUNT], float m_blur_sample_weights[m_BLUR_SAMPLE_COUNT])
{
	int center_i = m_BLUR_SAMPLE_COUNT / 2;
	int offset = 0;

	// Fill center element.
	m_blur_sample_offsets[center_i] = (float)offset;
	m_blur_sample_weights[center_i] = GuassianFunction1D((float)offset, m_blur_standard_deviation);
	
	// Fill right and left elements.
	for (offset = 1; offset < center_i + 1; offset++)
	{
		m_blur_sample_offsets[center_i + offset] = (float)offset;
		m_blur_sample_offsets[center_i - offset] = -(float)offset;
		m_blur_sample_weights[center_i + offset] = GuassianFunction1D((float)offset, m_blur_standard_deviation);
		m_blur_sample_weights[center_i - offset] = m_blur_sample_weights[center_i + offset];
	}
}

void PostProcessor::InitializeBlurDistributionForShader(float* sampleOffsets, float* sampleWeights, DirectX::XMFLOAT2 axis, float textureWidth, float textureHeight)
{
	// Copy into DirectX structures for horizontal and vertical passes.
	if (axis.x == 1 && axis.y == 0)
	{
		for (int offset_i = 0; offset_i < m_BLUR_SAMPLE_COUNT; offset_i++)
		{
			m_blur_sample_offsets_and_weights[offset_i].x = m_blur_sample_offsets[offset_i] / textureWidth;
			m_blur_sample_offsets_and_weights[offset_i].y = 0.0f;
			m_blur_sample_offsets_and_weights[offset_i].z = m_blur_sample_weights[offset_i];
		}
	}
	if (axis.x == 0 && axis.y == 1)
	{
		for (int offset_i = 0; offset_i < m_BLUR_SAMPLE_COUNT; offset_i++)
		{
			m_blur_sample_offsets_and_weights[offset_i].x = 0.0f;
			m_blur_sample_offsets_and_weights[offset_i].y = m_blur_sample_offsets[offset_i] / textureHeight;
			m_blur_sample_offsets_and_weights[offset_i].z = m_blur_sample_weights[offset_i];
		}
	}
}

void PostProcessor::Bloom(ID3D11ShaderResourceView* source_srv)
{
	mr_quad_renderer.SetVertexShader();

	// From an input texture, copy only pixels above a brightness threshold into an extraction texture.
	mup_bloom_extract_pixel_shader->SetShader();
	mup_bloom_extract_pixel_shader->SetSamplerState("sampler_linear", mcp_sampler_bilinear.Get());
	mup_bloom_extract_pixel_shader->SetShaderResourceView("source_texture", source_srv);
	mp_context->OMSetRenderTargets(1, mcp_bloom_extract_rtv.GetAddressOf(), 0);
	mr_quad_renderer.Draw(m_frame_buffer_desc.Width, m_frame_buffer_desc.Height);

	// Unbind resources
	mup_bloom_extract_pixel_shader->SetSamplerState("sampler_linear", 0);
	mup_bloom_extract_pixel_shader->SetShaderResourceView("source_texture", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);

	// Downsample the extraction texture four times into half by half textures
	mp_context->GenerateMips(mcp_bloom_extract_srv.Get());

	// Starting from the smallest, blur each downsample then upsample and add it to the next bigger downsample.
	for (int downsample_i = m_BLOOM_DOWNSAMPLE_COUNT - 1; downsample_i >= 0; downsample_i--)
	{ 
		mup_blur_pixel_shader->SetShader();
		mup_blur_pixel_shader->SetSamplerState("sampler_point", mcp_sampler_point.Get());

		D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)m_downsample_descs[downsample_i].Width, (float)m_downsample_descs[downsample_i].Height, 0.0f, 1.0f };
		mp_context->RSSetViewports(1, &vp);

		// Horizontal blur
		InitializeBlurDistributionForShader(m_blur_sample_offsets, m_blur_sample_weights, DirectX::XMFLOAT2(1, 0), m_downsample_descs[downsample_i].Width, m_downsample_descs[downsample_i].Height);
		mup_blur_pixel_shader->SetConstantBufferVariable("sample_offsets_and_weights", &m_blur_sample_offsets_and_weights[0], sizeof(m_blur_sample_offsets_and_weights));
		mup_blur_pixel_shader->UpdateAllConstantBuffers();
		mup_blur_pixel_shader->SetShaderResourceView("source_texture", mcp_bloom_vertical_blur_downsample_srvs[downsample_i].Get());
		mp_context->OMSetRenderTargets(1, mcp_bloom_horizontal_blur_downsample_rtvs[downsample_i].GetAddressOf(), 0);
		mr_quad_renderer.Draw();
		// Unbind resources
		mup_blur_pixel_shader->SetShaderResourceView("source_texture", 0);
		mp_context->OMSetRenderTargets(0, 0, 0);

		// Vertical blur
		InitializeBlurDistributionForShader(m_blur_sample_offsets, m_blur_sample_weights, DirectX::XMFLOAT2(0, 1), m_downsample_descs[downsample_i].Width, m_downsample_descs[downsample_i].Height);
		mup_blur_pixel_shader->SetConstantBufferVariable("sample_offsets_and_weights", &m_blur_sample_offsets_and_weights[0], sizeof(m_blur_sample_offsets_and_weights));
		mup_blur_pixel_shader->UpdateAllConstantBuffers();
		mup_blur_pixel_shader->SetShaderResourceView("source_texture", mcp_bloom_horizontal_blur_downsample_srvs[downsample_i].Get());
		mp_context->OMSetRenderTargets(1, mcp_bloom_vertical_blur_downsample_rtvs[downsample_i].GetAddressOf(), 0);
		mr_quad_renderer.Draw();
		// Unbind resources
		mup_blur_pixel_shader->SetSamplerState("sampler_point", 0);
		mup_blur_pixel_shader->SetShaderResourceView("source_texture", 0);
		mp_context->OMSetRenderTargets(0, 0, 0);

		// Add to next bigger downsample
		if (downsample_i > 0)
		{
			// Upsample and combine
			mup_bloom_combine_pixel_shader->SetShader();
			mup_bloom_combine_pixel_shader->SetSamplerState("sampler_bilinear", mcp_sampler_bilinear.Get());
			mup_bloom_combine_pixel_shader->SetShaderResourceView("source_texture", mcp_bloom_vertical_blur_downsample_srvs[downsample_i].Get());
			mp_context->OMSetRenderTargets(1, mcp_bloom_vertical_blur_downsample_rtvs[downsample_i - 1].GetAddressOf(), 0);
			mp_context->OMSetBlendState(mcp_bloom_combine_blend_state.Get(), 0, 0xffffffff);
			mr_quad_renderer.Draw(m_downsample_descs[downsample_i - 1].Width, m_downsample_descs[downsample_i - 1].Height);

			// Unbind resources
			mup_bloom_combine_pixel_shader->SetSamplerState("sampler_bilinear", 0);
			mup_bloom_combine_pixel_shader->SetShaderResourceView("source_texture", 0);
			mp_context->OMSetRenderTargets(0, 0, 0);
			mp_context->OMSetBlendState(0, 0, 0xffffffff);
		}
	}
	
	// Final blurred downsample is not combined with source until after Tone Mapping
}

void PostProcessor::InitializeToneMap()
{
	// Shaders
	mup_luminance_buffer_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_luminance_buffer_pixel_shader->InitializeShaderFromFile(L"x64/Debug/post_luminance_buffer_pixel.cso"))
		mup_luminance_buffer_pixel_shader->InitializeShaderFromFile(L"post_luminance_buffer_pixel.cso");
	mup_luminance_adapt_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_luminance_adapt_pixel_shader->InitializeShaderFromFile(L"x64/Debug/post_luminance_adapt_pixel.cso"))
		mup_luminance_adapt_pixel_shader->InitializeShaderFromFile(L"post_luminance_adapt_pixel.cso");
	mup_tone_map_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_tone_map_pixel_shader->InitializeShaderFromFile(L"x64/Debug/post_tone_map_pixel.cso"))
		mup_tone_map_pixel_shader->InitializeShaderFromFile(L"post_tone_map_pixel.cso");

	// Init Eye Adaptive Exposure
	// Luminance Buffer
	D3D11_TEXTURE2D_DESC luminanceDesc = {};
	luminanceDesc.Width = m_frame_buffer_desc.Width;
	luminanceDesc.Height = m_frame_buffer_desc.Height;
	luminanceDesc.MipLevels = 1;
	luminanceDesc.ArraySize = 1;
	luminanceDesc.Format = DXGI_FORMAT_R32_FLOAT;
	luminanceDesc.SampleDesc.Count = 1;
	luminanceDesc.SampleDesc.Quality = 0;
	luminanceDesc.Usage = D3D11_USAGE_DEFAULT;
	luminanceDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	luminanceDesc.CPUAccessFlags = 0;
	luminanceDesc.MiscFlags = 0;
	ID3D11Texture2D * luminanceT2d;
	mp_device->CreateTexture2D(&luminanceDesc, nullptr, &luminanceT2d);
	D3D11_RENDER_TARGET_VIEW_DESC luminanceRtvDesc = {};
	luminanceRtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	luminanceRtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	luminanceRtvDesc.Texture2D.MipSlice = 0;
	mp_device->CreateRenderTargetView(luminanceT2d, &luminanceRtvDesc, mcp_current_luminance_rtv.GetAddressOf());
	D3D11_SHADER_RESOURCE_VIEW_DESC luminanceSrvDesc = {};
	luminanceSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	luminanceSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	luminanceSrvDesc.Texture2D.MostDetailedMip = 0;
	luminanceSrvDesc.Texture2D.MipLevels = -1;
	mp_device->CreateShaderResourceView(luminanceT2d, &luminanceSrvDesc, mcp_current_luminance_srv.GetAddressOf());
	luminanceT2d->Release();
	// Adapted Luminance Buffer
	// Full mip chain required to reach 1x1 texture representing averageAdaptedLuminance
	m_adapted_luminance_texture_max_mip_level = floor(log2(m_frame_buffer_desc.Width) + 1) - 1;
	luminanceDesc.MipLevels = 0;
	luminanceDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	ID3D11Texture2D * adaptedLuminanceT2dSwap0;
	ID3D11Texture2D * adaptedLuminanceT2dSwap1;
	mp_device->CreateTexture2D(&luminanceDesc, nullptr, &adaptedLuminanceT2dSwap0);
	mp_device->CreateTexture2D(&luminanceDesc, nullptr, &adaptedLuminanceT2dSwap1);
	mp_device->CreateRenderTargetView(adaptedLuminanceT2dSwap0, &luminanceRtvDesc, mcp_adapted_luminance_rtvs[m_current_adapted_luminance_resource_i].GetAddressOf());
	mp_device->CreateRenderTargetView(adaptedLuminanceT2dSwap1, &luminanceRtvDesc, mcp_adapted_luminance_rtvs[!m_current_adapted_luminance_resource_i].GetAddressOf());
	// Full mip chain srv for GenerateMips
	mp_device->CreateShaderResourceView(adaptedLuminanceT2dSwap0, &luminanceSrvDesc, mcp_adapted_luminance_all_mips_srvs[m_current_adapted_luminance_resource_i].GetAddressOf());
	mp_device->CreateShaderResourceView(adaptedLuminanceT2dSwap1, &luminanceSrvDesc, mcp_adapted_luminance_all_mips_srvs[!m_current_adapted_luminance_resource_i].GetAddressOf());
	// Only first mip for rtv, GenerateMips will handle the rest
	luminanceSrvDesc.Texture2D.MipLevels = 1;
	mp_device->CreateShaderResourceView(adaptedLuminanceT2dSwap0, &luminanceSrvDesc, mcp_adapted_luminance_first_mip_srvs[m_current_adapted_luminance_resource_i].GetAddressOf());
	mp_device->CreateShaderResourceView(adaptedLuminanceT2dSwap1, &luminanceSrvDesc, mcp_adapted_luminance_first_mip_srvs[!m_current_adapted_luminance_resource_i].GetAddressOf());
	// Only last mip for averageAdaptedLuminance 1x1 texture
	luminanceSrvDesc.Texture2D.MostDetailedMip = m_adapted_luminance_texture_max_mip_level;
	mp_device->CreateShaderResourceView(adaptedLuminanceT2dSwap0, &luminanceSrvDesc, mcp_adapted_luminance_last_mip_srvs[m_current_adapted_luminance_resource_i].GetAddressOf());
	mp_device->CreateShaderResourceView(adaptedLuminanceT2dSwap1, &luminanceSrvDesc, mcp_adapted_luminance_last_mip_srvs[!m_current_adapted_luminance_resource_i].GetAddressOf());
	adaptedLuminanceT2dSwap0->Release();
	adaptedLuminanceT2dSwap1->Release();
}

void PostProcessor::GenerateAverageLuminance(ID3D11ShaderResourceView * pSourceTextureSrv, float pDeltaTime)
{
	mr_quad_renderer.SetVertexShader();
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)m_frame_buffer_desc.Width, (float)m_frame_buffer_desc.Height, 0.0f, 1.0f };
	mp_context->RSSetViewports(1, &vp);

	// Prep Eye Adaptive Exposure
	// Luminance Map
	mup_luminance_buffer_pixel_shader->SetShader();
	mup_luminance_buffer_pixel_shader->SetSamplerState("sampler_linear", mcp_sampler_trilinear.Get());
	mup_luminance_buffer_pixel_shader->SetShaderResourceView("source_texture", pSourceTextureSrv);
	mp_context->OMSetRenderTargets(1, mcp_current_luminance_rtv.GetAddressOf(), 0);
	mr_quad_renderer.Draw();
	mup_luminance_buffer_pixel_shader->SetSamplerState("sampler_linear", 0);
	mup_luminance_buffer_pixel_shader->SetShaderResourceView("source_texture", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);
	// Adapted Luminance Map
	mup_luminance_adapt_pixel_shader->SetShader();
	mup_luminance_adapt_pixel_shader->SetConstantBufferFloat("delta_time", pDeltaTime / 1000.0f);
	mup_luminance_adapt_pixel_shader->SetConstantBufferFloat("adaption_rate", .001f);
	mup_luminance_adapt_pixel_shader->UpdateAllConstantBuffers();
	mup_luminance_adapt_pixel_shader->SetSamplerState("sampler_point", mcp_sampler_point.Get());
	mup_luminance_adapt_pixel_shader->SetShaderResourceView("current_luminance_texture", mcp_current_luminance_srv.Get());
	mup_luminance_adapt_pixel_shader->SetShaderResourceView("past_adapted_luminance_texture", mcp_adapted_luminance_first_mip_srvs[!m_current_adapted_luminance_resource_i].Get());
	mp_context->OMSetRenderTargets(1, mcp_adapted_luminance_rtvs[m_current_adapted_luminance_resource_i].GetAddressOf(), 0);
	mr_quad_renderer.Draw();
	mup_luminance_adapt_pixel_shader->SetSamplerState("sampler_point", 0);
	mup_luminance_adapt_pixel_shader->SetShaderResourceView("current_luminance_texture", 0);
	mup_luminance_adapt_pixel_shader->SetShaderResourceView("past_adapted_luminance_texture", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);
	mp_context->GenerateMips(mcp_adapted_luminance_all_mips_srvs[m_current_adapted_luminance_resource_i].Get());
}

void PostProcessor::ToneMap(ID3D11ShaderResourceView * source_srv, ID3D11RenderTargetView * destination_rtv)
{
	mr_quad_renderer.SetVertexShader();
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)m_frame_buffer_desc.Width, (float)m_frame_buffer_desc.Height, 0.0f, 1.0f };
	mp_context->RSSetViewports(1, &vp);

	// Tone Map
	mup_tone_map_pixel_shader->SetShader();
	mup_tone_map_pixel_shader->SetSamplerState("sampler_point", mcp_sampler_point.Get());
	mup_tone_map_pixel_shader->SetSamplerState("sampler_bilinear", mcp_sampler_bilinear.Get());
	mup_tone_map_pixel_shader->SetShaderResourceView("source_texture", source_srv);
	mup_tone_map_pixel_shader->SetShaderResourceView("adapted_luminance_texture_max_mip", mcp_adapted_luminance_last_mip_srvs[m_current_adapted_luminance_resource_i].Get());
	mup_tone_map_pixel_shader->SetShaderResourceView("bloom_texture", mcp_bloom_vertical_blur_downsample_srvs[0].Get());
	mp_context->OMSetRenderTargets(1, &destination_rtv, 0);
	mr_quad_renderer.Draw();
	mup_tone_map_pixel_shader->SetSamplerState("sampler_point", 0);
	mup_tone_map_pixel_shader->SetSamplerState("sampler_bilinear", 0);
	mup_tone_map_pixel_shader->SetShaderResourceView("source_texture", 0);
	mup_tone_map_pixel_shader->SetShaderResourceView("adapted_luminance_texture_max_mip", 0);
	mup_tone_map_pixel_shader->SetShaderResourceView("bloom_texture", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);

	// Swap past / current adapted luminance texture
	m_current_adapted_luminance_resource_i = !m_current_adapted_luminance_resource_i;
}
