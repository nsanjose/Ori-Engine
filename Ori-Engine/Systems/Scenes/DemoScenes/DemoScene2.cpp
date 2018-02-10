#include "DemoScene2.h"

#include "..\..\..\Components\CameraComponent.h"
#include "..\..\..\Components\LightComponent.h"
#include "..\..\..\Components\ParticleEmitterComponent.h"

using namespace DirectX;

DemoScene2::DemoScene2(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, float frame_width, float frame_height,
	ImageBasedLightingBaker* ibl_baker, ShadowRenderer* shadow_renderer)
{
	is_tone_mapped = false;
	enable_ssao = false;

	Entity* camera = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(1, 2, -3),		// Position
			XMFLOAT3(0.2, 0, 0)));	// Rotation
	camera->AddComponent(std::make_unique<CameraComponent>(
		camera->GetTransformComponent(),
		0.25f * XM_PI,						// FOV
		(float)frame_width / frame_height,	// Aspect Ratio
		0.1f,								// Near Clip
		100.0f));							// Far Clip
	//AddCamera();
	SetCurrentCamera(camera);

	Entity* sun = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 10, 0),			// Position
			XMFLOAT3(1, -1, 1)));		// Rotation
	sun->AddComponent(std::make_unique<LightComponent>(
		std::make_unique<DirectionalLight>(
			XMFLOAT3(1.0f, 1.0f, 1.0f),	// Color
			2.0f)));					// Irradiance
	shadow_renderer->EnableShadowing(*sun);
	shadow_renderer->EnableCascadedShadowing(*sun, 4);
	AddLight(sun);

	std::shared_ptr<Mesh> cube_mesh = std::make_shared<Mesh>(pp_device, "Resources/Mesh Files/cube.obj");
	std::shared_ptr<Mesh> sphere_mesh = std::make_shared<Mesh>(pp_device, "Resources/Mesh Files/sphere.obj");

	Entity* shadow_floor = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 0, 0),				// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(20.0f, 0.1f, 20.0f)));	// Scale
	shadow_floor->AddComponent(std::make_unique<DrawComponent>(
		cube_mesh,
			std::make_shared<Material>(
			XMFLOAT3(1, 1, 1),
			0.f,		// Metalness
			1.f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(shadow_floor);

	Entity* test_sphere_00 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 2, 0),				// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_00->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			XMFLOAT3(1, 1, 1),
			0.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_00);
}

DemoScene2::~DemoScene2()
{
}
