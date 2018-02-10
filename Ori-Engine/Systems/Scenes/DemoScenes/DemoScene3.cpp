#include "DemoScene3.h"

#include "..\..\..\Components\CameraComponent.h"
#include "..\..\..\Components\LightComponent.h"
#include "..\..\..\Components\ParticleEmitterComponent.h"

using namespace DirectX;

DemoScene3::DemoScene3(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, float frame_width, float frame_height,
	ImageBasedLightingBaker* ibl_baker, ShadowRenderer* shadow_renderer)
{
	is_tone_mapped = false;
	enable_ssao = true;

	SkyBox * skybox = new SkyBox();
	skybox->LoadEnvMap(*pp_device, L"Resources/Cube Maps/Humus/Tenerife.dds");
	skybox->LoadIrradianceMap(*pp_device, L"Resources/Cube Maps/Humus/Tenerife_Irradiance.dds");
	ibl_baker->GeneratePfem(skybox);
	ibl_baker->GenerateBrdfLut(skybox);
	SetCurrentSkyBox(skybox);

	Entity* camera = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-3, 2.5, -2.5),	// Position
			XMFLOAT3(0.5, 0.9, 1)));	// Rotation
	camera->AddComponent(std::make_unique<CameraComponent>(
		camera->GetTransformComponent(),
		0.25f * XM_PI,						// FOV
		(float)frame_width / frame_height,	// Aspect Ratio
		0.1f,								// Near Clip
		100.0f));							// Far Clip
	//AddCamera();
	SetCurrentCamera(camera);

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
			1.f,		// Metalness
			1.f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(shadow_floor);

	Entity* test_cube_00 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 0, 0),			// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(2.5, .5, 2.5)));	// Scale
	test_cube_00->AddComponent(std::make_unique<DrawComponent>(
		cube_mesh,
		std::make_shared<Material>(
			XMFLOAT3(1, 1, 1),
			1.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_cube_00);

	Entity* test_cube_01 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(1, 0, 0),		// Position
			XMFLOAT3(0, 0, 0),		// Rotation
			XMFLOAT3(2, 2, 2)));	// Scale
	test_cube_01->AddComponent(std::make_unique<DrawComponent>(
		cube_mesh,
		std::make_shared<Material>(
			XMFLOAT3(1, 1, 1),
			1.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_cube_01);

	Entity* test_cube_02 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(1, 1, 1),		// Position
			XMFLOAT3(0, 0, 0),		// Rotation
			XMFLOAT3(1, 1, 1)));	// Scale
	test_cube_02->AddComponent(std::make_unique<DrawComponent>(
		cube_mesh,
		std::make_shared<Material>(
			XMFLOAT3(1, 1, 1),
			1.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_cube_02);

	Entity* test_cube_03 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(1, 0, 1),				// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_cube_03->AddComponent(std::make_unique<DrawComponent>(
		cube_mesh,
		std::make_shared<Material>(
			XMFLOAT3(1, 1, 1),
			1.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_cube_03);

	Entity* test_cube_04 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-1, 0, 1),		// Position
			XMFLOAT3(0, 0, 0),		// Rotation
			XMFLOAT3(1, 1, 1)));	// Scale
	test_cube_04->AddComponent(std::make_unique<DrawComponent>(
		cube_mesh,
		std::make_shared<Material>(
			XMFLOAT3(1, 1, 1),
			1.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_cube_04);
}

DemoScene3::~DemoScene3()
{
}
