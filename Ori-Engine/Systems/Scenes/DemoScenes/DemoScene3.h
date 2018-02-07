#pragma once

#include "..\Scene.h"

#include "..\..\Graphics\Tools\ImageBasedLightingBaker.h"
#include "..\..\Graphics\Tools\ShadowRenderer.h"

class DemoScene3 : public Scene
{
public:
	DemoScene3(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, float frame_width, float frame_height,
		ImageBasedLightingBaker* ibl_baker, ShadowRenderer* shadow_renderer);
	~DemoScene3();

private:
};