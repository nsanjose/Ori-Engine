#include "ImageBasedLightingBaker.h"

ImageBasedLightingBaker::ImageBasedLightingBaker(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, QuadRenderer& pr_quad_renderer)
	: mp_device(pp_device), mp_context(pp_context), mr_quad_renderer(pr_quad_renderer)
{
	mup_bake_pfem_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_bake_pfem_pixel_shader->InitializeShaderFromFile(L"x64/Debug/bake_pfem_pixel.cso"))
		mup_bake_pfem_pixel_shader->InitializeShaderFromFile(L"bake_pfem_pixel.cso");
	mup_bake_brdf_lut_pixel_shader = std::make_unique<PixelShader>(mp_device, mp_context);
	if (!mup_bake_brdf_lut_pixel_shader->InitializeShaderFromFile(L"x64/Debug/bake_brdf_lut_pixel.cso"))
		mup_bake_brdf_lut_pixel_shader->InitializeShaderFromFile(L"bake_brdf_lut_pixel.cso");

	D3D11_SAMPLER_DESC sampler_desc = {};
	ZeroMemory(&sampler_desc, sizeof(D3D11_SAMPLER_DESC));
	sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_desc.MaxAnisotropy = 16;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
	mp_device->CreateSamplerState(&sampler_desc, mcp_sampler_ansio.GetAddressOf());
	std::string mcp_sampler_ansio_name("Ansio Border Sampler");
	mcp_sampler_ansio.Get()->SetPrivateData(WKPDID_D3DDebugObjectName, mcp_sampler_ansio_name.size(), mcp_sampler_ansio_name.c_str());
}

ImageBasedLightingBaker::~ImageBasedLightingBaker()
{
}

// =====================================================================================================
//		Generate Prefiltered Environment Map for IBL
// =====================================================================================================
void ImageBasedLightingBaker::GeneratePfem(SkyBox* pp_skybox)
{
	mr_quad_renderer.SetVertexShader();
	mup_bake_pfem_pixel_shader->SetShader();

	mup_bake_pfem_pixel_shader->SetShaderResourceView("env_map", pp_skybox->GetEnvMapSrv());
	mup_bake_pfem_pixel_shader->SetSamplerState("sampler_ansio", mcp_sampler_ansio.Get());

	int textureWidth = 256;
	int mipCount = 7;
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width				= textureWidth;
	textureDesc.Height				= textureWidth;
	textureDesc.MipLevels			= mipCount;
	textureDesc.ArraySize			= 6;
	textureDesc.SampleDesc.Count	= 1;
	textureDesc.SampleDesc.Quality	= 0;
	textureDesc.Format				= DXGI_FORMAT_R16G16B16A16_FLOAT;
	textureDesc.Usage				= D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags			= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags		= 0;
	textureDesc.MiscFlags			= D3D11_RESOURCE_MISC_TEXTURECUBE;
	ID3D11Texture2D* pfemT2d;// = pp_skybox->GetPrefilteredEnvMapT2d();
	HRESULT hr = mp_device->CreateTexture2D(&textureDesc, nullptr, &pfemT2d);
	if (FAILED(hr)) { throw std::exception(); }
	std::string pfem_texture_name("PFEM Cube Map Texture");
	pfemT2d->SetPrivateData(WKPDID_D3DDebugObjectName, pfem_texture_name.size(), pfem_texture_name.c_str());

	for (int faceIndex = 0; faceIndex < 6; faceIndex++)
	{
		mup_bake_pfem_pixel_shader->SetConstantBufferInt("face_i", faceIndex);
		textureWidth = textureDesc.Width;

		for (int mipIndex = 0; mipIndex < mipCount; mipIndex++)
		{
			mup_bake_pfem_pixel_shader->SetConstantBufferInt("mip_i", mipIndex);
			mup_bake_pfem_pixel_shader->UpdateAllConstantBuffers();

			// Mip specific rtv per cubemap face
			D3D11_RENDER_TARGET_VIEW_DESC renderTargetDesc = {};
			ZeroMemory(&renderTargetDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
			renderTargetDesc.Format = textureDesc.Format;
			renderTargetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			renderTargetDesc.Texture2DArray.MipSlice = mipIndex;
			renderTargetDesc.Texture2DArray.ArraySize = 1;
			renderTargetDesc.Texture2DArray.FirstArraySlice = faceIndex;
			ID3D11RenderTargetView* rtv;
			HRESULT hr = mp_device->CreateRenderTargetView(pfemT2d, &renderTargetDesc, &rtv);
			if (FAILED(hr)) { throw std::exception(); }
			std::string pfem_rtv_name("PFEM Cube Map RTV");
			rtv->SetPrivateData(WKPDID_D3DDebugObjectName, pfem_rtv_name.size(), pfem_rtv_name.c_str());

			mp_context->OMSetRenderTargets(1, &rtv, 0);
			float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			mp_context->ClearRenderTargetView(rtv, clearColor);

			mr_quad_renderer.Draw(textureWidth, textureWidth);
			rtv->Release();

			// Half texture size for each mip level
			textureWidth /= 2;
		}
	}

	mup_bake_pfem_pixel_shader->SetShaderResourceView("env_map", 0);
	mup_bake_pfem_pixel_shader->SetSamplerState("sampler_ansio", 0);
	mp_context->OMSetRenderTargets(0, 0, 0);
	mp_context->RSSetState(0);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
	pp_skybox->CreatePfemSrv(mp_device, pfemT2d, &srvDesc);
	//mp_device->CreateShaderResourceView(pfemT2d, &srvDesc, prcp_PfemSrv.GetAddressOf());

	pfemT2d->Release();
}

// =====================================================================================================
//		Generate BRDF Look Up Table (LUT) for IBL
// =====================================================================================================
void ImageBasedLightingBaker::GenerateBrdfLut(SkyBox* pp_skybox)
{
	mr_quad_renderer.SetVertexShader();
	mup_bake_brdf_lut_pixel_shader->SetShader();

	int textureWidth = 256;
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = textureWidth;
	textureDesc.Height = textureWidth;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	ID3D11Texture2D* brdfLutT2d;
	mp_device->CreateTexture2D(&textureDesc, nullptr, &brdfLutT2d);
	std::string brdf_lut_name("BRDF LUT");
	brdfLutT2d->SetPrivateData(WKPDID_D3DDebugObjectName, brdf_lut_name.size(), brdf_lut_name.c_str());
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetDesc = {};
	renderTargetDesc.Format = textureDesc.Format;
	renderTargetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetDesc.Texture2D.MipSlice = 0;
	ID3D11RenderTargetView* renderTargetView;
	mp_device->CreateRenderTargetView(brdfLutT2d, &renderTargetDesc, &renderTargetView);
	std::string brdf_lut_rtv_name("BRDF LUT RTV");
	renderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName, brdf_lut_rtv_name.size(), brdf_lut_rtv_name.c_str());

	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	mp_context->OMSetRenderTargets(1, &renderTargetView, NULL);
	mp_context->ClearRenderTargetView(renderTargetView, clearColor);

	mr_quad_renderer.Draw((float)textureWidth, (float)textureWidth);

	mp_context->OMSetRenderTargets(0, 0, 0);
	mp_context->RSSetState(0);

	renderTargetView->Release();

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
	//mp_device->CreateShaderResourceView(pp_BrdfLutT2d, &srvDesc, pp_BrdfLutSrv.GetAddressOf());
	pp_skybox->CreateBrdfLutSrv(mp_device, brdfLutT2d, &srvDesc);

	brdfLutT2d->Release();
}