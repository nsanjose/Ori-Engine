#pragma once

#include "..\Scene.h"

#include "..\..\Graphics\Tools\ImageBasedLightingBaker.h"
#include "..\..\Graphics\Tools\ShadowRenderer.h"

class DemoScene2 : public Scene
{
public:
	DemoScene2(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, float frame_width, float frame_height,
		ImageBasedLightingBaker* ibl_baker, ShadowRenderer* shadow_renderer);
	~DemoScene2();

private:
};