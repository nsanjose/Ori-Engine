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
	D3D11_SAMPLER_DESC sampler_linear_desc = {};
	ZeroMemory(&sampler_linear_desc, sizeof(D3D11_SAMPLER_DESC));
	sampler_linear_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_linear_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_linear_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_linear_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	mp_device->CreateSamplerState(&sampler_linear_desc, mcp_sampler_linear.GetAddressOf());

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

ID3D11RenderTargetView* PostProcessor::GetFrameBuffer2Rtv() const
{
	return mcp_frame_buffer2_rtv.Get();
}

ID3D11ShaderResourceView* PostProcessor::GetFrameBuffer2Srv() const
{
	return mcp_frame_buffer2_srv.Get();
}

void PostProcessor::InitializeFrameBuffers(float pWidth, float pHeight)
{
	m_frame_buffer_desc.Width					= pWidth;
	m_frame_buffer_desc.Height				= pHeight;
	m_frame_buffer_desc.MipLevels				= 1;
	m_frame_buffer_desc.ArraySize				= 1;
	m_frame_buffer_desc.Format				= DXGI_FORMAT_R16G16B16A16_FLOAT;//DXGI_FORMAT_R32G32B32A32_FLOAT;
	m_frame_buffer_desc.SampleDesc.Count		= 1;
	m_frame_buffer_desc.SampleDesc.Quality	= 0;
	m_frame_buffer_desc.Usage					= D3D11_USAGE_DEFAULT;
	m_frame_buffer_desc.BindFlags				= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	m_frame_buffer_desc.CPUAccessFlags		= 0;
	m_frame_buffer_desc.MiscFlags				= 0;
	ID3D11Texture2D* frameBuffer1;
	ID3D11Texture2D* frameBuffer2;
	mp_device->CreateTexture2D(&m_frame_buffer_desc, nullptr, &frameBuffer1);
	mp_device->CreateTexture2D(&m_frame_buffer_desc, nullptr, &frameBuffer2);

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format							= m_frame_buffer_desc.Format;
	rtvDesc.ViewDimension					= D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice				= 0;
	mp_device->CreateRenderTargetView(frameBuffer1, &rtvDesc, mcp_frame_buffer_rtv.GetAddressOf());
	mp_device->CreateRenderTargetView(frameBuffer2, &rtvDesc, mcp_frame_buffer2_rtv.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = m_frame_buffer_desc.Format;
	srvDesc.ViewDimension					= D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels				= 1;
	srvDesc.Texture2D.MostDetailedMip		= 0;
	mp_device->CreateShaderResourceView(frameBuffer1, &srvDesc, mcp_frame_buffer_srv.GetAddressOf());
	mp_device->CreateShaderResourceView(frameBuffer2, &srvDesc, mcp_frame_buffer2_srv.GetAddressOf());

	frameBuffer1->Release();
	frameBuffer2->Release();
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
	if (!mup_bloom_extract_pixel_shader->InitializeShaderFromFile(L"x64/Debug/post_bloom_combine_pixel.cso"))
		mup_bloom_extract_pixel_shader->InitializeShaderFromFile(L"post_bloom_combine_pixel.cso");
	mup_bloom_combine_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_bloom_combine_pixel_shader->InitializeShaderFromFile(L"x64/Debug/post_bloom_combine_pixel.cso"))
		mup_bloom_combine_pixel_shader->InitializeShaderFromFile(L"post_bloom_combine_pixel.cso");

	// Extraction texture resources
	D3D11_TEXTURE2D_DESC t2ddescBloomExtract = {};
	t2ddescBloomExtract.Width = m_frame_buffer_desc.Width;
	t2ddescBloomExtract.Height = m_frame_buffer_desc.Height;
	t2ddescBloomExtract.MipLevels = 1;
	t2ddescBloomExtract.ArraySize = 1;
	t2ddescBloomExtract.Format = m_frame_buffer_desc.Format;
	t2ddescBloomExtract.SampleDesc.Count = 1;
	t2ddescBloomExtract.SampleDesc.Quality = 0;
	t2ddescBloomExtract.Usage = D3D11_USAGE_DEFAULT;
	t2ddescBloomExtract.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	t2ddescBloomExtract.CPUAccessFlags = 0;
	t2ddescBloomExtract.MiscFlags = 0;
	ID3D11Texture2D* bloomExtractT2d;
	mp_device->CreateTexture2D(&t2ddescBloomExtract, nullptr, &bloomExtractT2d);

	D3D11_RENDER_TARGET_VIEW_DESC adaptiveExposureRtvDesc = {};
	adaptiveExposureRtvDesc.Format = t2ddescBloomExtract.Format;
	adaptiveExposureRtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	adaptiveExposureRtvDesc.Texture2D.MipSlice = 0;
	mp_device->CreateRenderTargetView(bloomExtractT2d, &adaptiveExposureRtvDesc, mcp_bloom_extract_rtv.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvdescBloomExtract = {};
	srvdescBloomExtract.Format = t2ddescBloomExtract.Format;
	srvdescBloomExtract.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdescBloomExtract.Texture2D.MostDetailedMip = 0;
	srvdescBloomExtract.Texture2D.MipLevels = t2ddescBloomExtract.MipLevels;
	mp_device->CreateShaderResourceView(bloomExtractT2d, &srvdescBloomExtract, mcp_bloom_extract_srv.GetAddressOf());
	bloomExtractT2d->Release();

	// Downsample/Blur textures' resources
	const int mNumDownsamples = 1;	//4	//move to .h
	unsigned int downsampleDivisor = 1;	//2
	for (int downsample_i = 0; downsample_i < mNumDownsamples; downsample_i++)
	{
		// Each downsample's dimensions are halved.
		//downsampleDivisor *= 2;
		m_bloom_blur_downsample_descs.push_back(D3D11_TEXTURE2D_DESC());
		m_bloom_blur_downsample_descs[downsample_i].Width = m_frame_buffer_desc.Width / downsampleDivisor;
		m_bloom_blur_downsample_descs[downsample_i].Height = m_frame_buffer_desc.Height / downsampleDivisor;
		m_bloom_blur_downsample_descs[downsample_i].MipLevels = 1;
		m_bloom_blur_downsample_descs[downsample_i].ArraySize = 1;
		m_bloom_blur_downsample_descs[downsample_i].Format = m_frame_buffer_desc.Format;
		m_bloom_blur_downsample_descs[downsample_i].SampleDesc.Count = 1;
		m_bloom_blur_downsample_descs[downsample_i].SampleDesc.Quality = 0;
		m_bloom_blur_downsample_descs[downsample_i].Usage = D3D11_USAGE_DEFAULT;
		m_bloom_blur_downsample_descs[downsample_i].BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		m_bloom_blur_downsample_descs[downsample_i].CPUAccessFlags = 0;
		m_bloom_blur_downsample_descs[downsample_i].MiscFlags = 0;

		ID3D11Texture2D* pt2dBlurPass;
		ID3D11Texture2D* horizontalBlurInterimT2d;
		mp_device->CreateTexture2D(&m_bloom_blur_downsample_descs[downsample_i], nullptr, &pt2dBlurPass);
		mp_device->CreateTexture2D(&m_bloom_blur_downsample_descs[downsample_i], nullptr, &horizontalBlurInterimT2d);

		D3D11_RENDER_TARGET_VIEW_DESC rtvdescBlurPass = {};
		rtvdescBlurPass.Format = m_bloom_blur_downsample_descs[downsample_i].Format;
		rtvdescBlurPass.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvdescBlurPass.Texture2D.MipSlice = 0;
		mcp_bloom_blur_downsample_rtvs.push_back(Microsoft::WRL::ComPtr<ID3D11RenderTargetView>());
		mcp_bloom_blur_downsample_temp_rtvs.push_back(Microsoft::WRL::ComPtr<ID3D11RenderTargetView>());
		mp_device->CreateRenderTargetView(pt2dBlurPass, &adaptiveExposureRtvDesc, mcp_bloom_blur_downsample_rtvs.back().GetAddressOf());
		mp_device->CreateRenderTargetView(horizontalBlurInterimT2d, &adaptiveExposureRtvDesc, mcp_bloom_blur_downsample_temp_rtvs.back().GetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC srvdescBlurPass = {};
		srvdescBlurPass.Format = m_bloom_blur_downsample_descs[downsample_i].Format;
		srvdescBlurPass.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvdescBlurPass.Texture2D.MostDetailedMip = 0;
		srvdescBlurPass.Texture2D.MipLevels = 1;
		mcp_bloom_blur_downsample_srvs.push_back(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>());
		mcp_bloom_blur_downsample_temp_srvs.push_back(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>());
		mp_device->CreateShaderResourceView(pt2dBlurPass, &srvdescBlurPass, mcp_bloom_blur_downsample_srvs.back().GetAddressOf());
		mp_device->CreateShaderResourceView(horizontalBlurInterimT2d, &srvdescBlurPass, mcp_bloom_blur_downsample_temp_srvs.back().GetAddressOf());

		// Texture references are not needed. This does not unallocate the texture memory.
		pt2dBlurPass->Release();
		horizontalBlurInterimT2d->Release();
	}
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

void PostProcessor::Bloom(ID3D11ShaderResourceView* sourceSrv)
{
	mr_quad_renderer.SetVertexShader();

	// From an input texture, copy only pixels above a brightness threshold into an extraction texture.
	mup_bloom_extract_pixel_shader->SetShader();
	mup_bloom_extract_pixel_shader->SetSamplerState("sampler_linear", mcp_sampler_linear.Get());
	mup_bloom_extract_pixel_shader->SetShaderResourceView("source_texture", sourceSrv);
	mp_context->OMSetRenderTargets(1, mcp_bloom_extract_rtv.GetAddressOf(), 0);
	mr_quad_renderer.Draw(m_frame_buffer_desc.Width, m_frame_buffer_desc.Height);

	// Unbind resources
	mup_bloom_extract_pixel_shader->SetSamplerState("sampler_linear", 0);
	mup_bloom_extract_pixel_shader->SetShaderResourceView("source_texture", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);

	// Downsample the extraction texture four times into half by half textures.
	//mp_context->GenerateMips(psrvBloomExtract);	

	// Starting from the smallest, blur each downsample then upsample and add it to the next bigger downsample.
	for (int downsample_i = (int)mcp_bloom_blur_downsample_rtvs.size() - 1; downsample_i >= 0; downsample_i--)
	{ 
		mup_blur_pixel_shader->SetShader();
		mup_blur_pixel_shader->SetSamplerState("sampler_point", mcp_sampler_point.Get());
		//mup_blur_pixel_shader->SetShaderResourceView("t2dSource", mcp_bloom_extract_srv.Get());

		// Horizontal blur
		InitializeBlurDistributionForShader(m_blur_sample_offsets, m_blur_sample_weights, DirectX::XMFLOAT2(1, 0), m_frame_buffer_desc.Width, m_frame_buffer_desc.Height);
		mup_blur_pixel_shader->SetConstantBufferVariable("sample_offsets_and_weights", &m_blur_sample_offsets_and_weights[0], sizeof(m_blur_sample_offsets_and_weights));
		mup_blur_pixel_shader->UpdateAllConstantBuffers();
		//mup_blur_pixel_shader->SetShaderResourceView("t2dSource", mcp_bloom_blur_downsample_srvs[downsample_i].Get());
		mup_blur_pixel_shader->SetShaderResourceView("source_texture", mcp_bloom_extract_srv.Get());
		mp_context->OMSetRenderTargets(1, mcp_bloom_blur_downsample_temp_rtvs[downsample_i].GetAddressOf(), 0);
		mr_quad_renderer.Draw(m_bloom_blur_downsample_descs[downsample_i].Width, m_bloom_blur_downsample_descs[downsample_i].Height);
		// Unbind resources
		mup_blur_pixel_shader->SetShaderResourceView("source_texture", 0);
		mp_context->OMSetRenderTargets(0, 0, 0);

		// Vertical blur
		InitializeBlurDistributionForShader(m_blur_sample_offsets, m_blur_sample_weights, DirectX::XMFLOAT2(0, 1), m_frame_buffer_desc.Width, m_frame_buffer_desc.Height);
		mup_blur_pixel_shader->SetConstantBufferVariable("sample_offsets_and_weights", &m_blur_sample_offsets_and_weights[0], sizeof(m_blur_sample_offsets_and_weights));
		mup_blur_pixel_shader->UpdateAllConstantBuffers();
		mup_blur_pixel_shader->SetShaderResourceView("source_texture", mcp_bloom_blur_downsample_temp_srvs[downsample_i].Get());
		mp_context->OMSetRenderTargets(1, mcp_bloom_blur_downsample_rtvs[downsample_i].GetAddressOf(), 0);
		mr_quad_renderer.Draw(m_bloom_blur_downsample_descs[downsample_i].Width, m_bloom_blur_downsample_descs[downsample_i].Height);	
		// Unbind resources
		mup_blur_pixel_shader->SetSamplerState("sampler_point", 0);
		mup_blur_pixel_shader->SetShaderResourceView("source_texture", 0);
		mp_context->OMSetRenderTargets(0, 0, 0);

		// Add to next bigger downsample
		/*
		if (downsample_i > 0)
		{
			mup_bloom_combine_pixel_shader->SetShader();
			mup_bloom_combine_pixel_shader->SetSamplerState("sampler_pointBorder", m_cpSamplerPointBorder.Get());
			mup_bloom_combine_pixel_shader->SetShaderResourceView("sourceOne", mcp_bloom_blur_downsample_srvs[downsample_i - 1].Get());
			mup_bloom_combine_pixel_shader->SetShaderResourceView("sourceTwo", mcp_bloom_blur_downsample_srvs[downsample_i].Get());
			mp_context->OMSetRenderTargets(1, , 0);
			mr_quad_renderer->Draw(m_bloom_blur_downsample_descs[downsample_i - 1].Width, m_bloom_blur_downsample_descs[downsample_i - 1].Height);
		}
		*/
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
	mup_luminance_buffer_pixel_shader->SetSamplerState("sampler_linear", mcp_sampler_linear.Get());
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

void PostProcessor::ToneMap(ID3D11ShaderResourceView * pSourceTextureSrv, ID3D11RenderTargetView * pDestinationTextureRtv)
{
	mr_quad_renderer.SetVertexShader();
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)m_frame_buffer_desc.Width, (float)m_frame_buffer_desc.Height, 0.0f, 1.0f };
	mp_context->RSSetViewports(1, &vp);

	// Tone Map
	mup_tone_map_pixel_shader->SetShader();
	mup_tone_map_pixel_shader->SetSamplerState("sampler_point", mcp_sampler_point.Get());
	mup_tone_map_pixel_shader->SetSamplerState("sampler_linear", mcp_sampler_linear.Get());
	mup_tone_map_pixel_shader->SetShaderResourceView("source_texture", pSourceTextureSrv);
	mup_tone_map_pixel_shader->SetShaderResourceView("adapted_luminance_texture_max_mip", mcp_adapted_luminance_last_mip_srvs[m_current_adapted_luminance_resource_i].Get());
	mup_tone_map_pixel_shader->SetShaderResourceView("bloom_texture", mcp_bloom_blur_downsample_srvs[0].Get());
	mp_context->OMSetRenderTargets(1, &pDestinationTextureRtv, 0);
	mr_quad_renderer.Draw();
	mup_tone_map_pixel_shader->SetSamplerState("sampler_point", 0);
	mup_tone_map_pixel_shader->SetSamplerState("sampler_linear", 0);
	mup_tone_map_pixel_shader->SetShaderResourceView("source_texture", 0);
	mup_tone_map_pixel_shader->SetShaderResourceView("adapted_luminance_texture_max_mip", 0);
	mup_tone_map_pixel_shader->SetShaderResourceView("bloom_texture", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);

	// Swap past / current adapted luminance texture
	m_current_adapted_luminance_resource_i = !m_current_adapted_luminance_resource_i;
}
