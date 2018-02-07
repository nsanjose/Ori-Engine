#pragma once

#include "..\Scene.h"

#include "..\..\Graphics\Tools\ImageBasedLightingBaker.h"
#include "..\..\Graphics\Tools\ShadowRenderer.h"

class DemoScene : public Scene
{
public:
	DemoScene(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, float frame_width, float frame_height,
		ImageBasedLightingBaker* ibl_baker, ShadowRenderer* shadow_renderer);
	~DemoScene();

private:
};