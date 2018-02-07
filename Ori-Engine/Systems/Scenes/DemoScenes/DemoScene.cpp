#include "DemoScene.h"

#include "..\..\..\Components\CameraComponent.h"
#include "..\..\..\Components\LightComponent.h"
#include "..\..\..\Components\ParticleEmitterComponent.h"

using namespace DirectX;

DemoScene::DemoScene(ID3D11Device* pp_device, ID3D11DeviceContext* pp_context, float frame_width, float frame_height,
	ImageBasedLightingBaker* ibl_baker, ShadowRenderer* shadow_renderer)
{
	SkyBox * skybox = new SkyBox();
	skybox->LoadEnvMap(*pp_device, L"Resources/Cube Maps/Humus/Stairs.dds");
	skybox->LoadIrradianceMap(*pp_device, L"Resources/Cube Maps/Humus/Stairs_Irradiance.dds");
	ibl_baker->GeneratePfem(skybox);
	ibl_baker->GenerateBrdfLut(skybox);
	SetCurrentSkyBox(skybox);

	Entity* camera = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 2, -8),		// Position
			XMFLOAT3(0, 0, 1)));	// Rotation
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
			XMFLOAT3(0, 5, 0),					// Position
			XMFLOAT3(0, 0.000000001, 1)));		// Rotation
	sun->AddComponent(std::make_unique<LightComponent>(
		std::make_unique<DirectionalLight>(
			XMFLOAT3(1.0f, 1.0f, 1.0f),	// Color
			2.0f)));					// Irradiance
	AddLight(sun);

	Entity* emitter = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 15.f, -2.2f))); // Position
	emitter->AddComponent(std::make_unique<ParticleEmitterComponent>(
		emitter->GetTransformComponent(),
		pp_device));
	AddEntity(emitter);

	//std::shared_ptr<Mesh> cube_mesh = std::make_shared<Mesh>(pp_device, "Resources/Mesh Files/cube.obj");
	std::shared_ptr<Mesh> sphere_mesh = std::make_shared<Mesh>(pp_device, "Resources/Mesh Files/sphere.obj");

	Entity* wood_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-4.4f, 0, 0),			// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	wood_sphere->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(pp_device, pp_context,
			L"Resources/Texture Files/OakFloor/oakfloor_basecolor.png",		// Diffuse
			L"Resources/Texture Files/Blanks/Full_0.png",					// Metallic
			L"Resources/Texture Files/OakFloor/oakfloor_roughness.png",		// Roughness
			L"Resources/Texture Files/OakFloor/oakfloor_normal.png",		// Normal
			L"Resources/Texture Files/OakFloor/oakfloor_AO.png")));			// AO
	AddEntity(wood_sphere);

	Entity* rock_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-2.2f, 0, 0),			// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
		rock_sphere->AddComponent(std::make_unique<DrawComponent>(
			sphere_mesh,
				std::make_shared<Material>(pp_device, pp_context,
				L"Resources/Texture Files/Rock/TexturesCom_Rock_1024_albedo.tif",		// Diffuse
				L"Resources/Texture Files/Blanks/Full_0.png",							// Metallic
				L"Resources/Texture Files/Rock/TexturesCom_Rock_1024_roughness.tif",	// Roughness
				L"Resources/Texture Files/Rock/TexturesCom_Rock_1024_normal.tif",		// Normal
				L"Resources/Texture Files/Rock/TexturesCom_Rock_1024_ao.tif")));		// AO
	AddEntity(rock_sphere);

	Entity* brick_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 0, 0),				// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	brick_sphere->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(pp_device, pp_context,
			L"Resources/Texture Files/Bricks/brick_wall_001_baseColor.png",				// Diffuse
			L"Resources/Texture Files/Bricks/brick_wall_001_metallic.png",				// Metallic
			L"Resources/Texture Files/Bricks/brick_wall_001_roughness.png",				// Roughness
			L"Resources/Texture Files/Bricks/brick_wall_001_normal.png",				// Normal
			L"Resources/Texture Files/Bricks/brick_wall_001_ambientOcclusion.png")));	// AO
	AddEntity(brick_sphere);

	Entity* rust_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(2.2f, 0, 0),			// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	rust_sphere->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(pp_device, pp_context,
			L"Resources/Texture Files/RustedIron/rustediron2_basecolor.png",			// Diffuse
			L"Resources/Texture Files/RustedIron/rustediron2_metallic.png",				// Metallic
			L"Resources/Texture Files/RustedIron/rustediron2_roughness.png",			// Roughness
			L"Resources/Texture Files/RustedIron/rustediron2_normal.png",				// Normal
			L"Resources/Texture Files/RustedIron/rustediron2_ambientOcclusion.png")));	// AO
	AddEntity(rust_sphere);

	Entity* gold_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(4.4f, 0, 0),			// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	gold_sphere->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(pp_device, pp_context,
			L"Resources/Texture Files/Gold/gold_baseColor.png",				// Diffuse
			L"Resources/Texture Files/Gold/gold_metallic.png",				// Metallic
			L"Resources/Texture Files/Gold/gold_roughness.png",				// Roughness
			L"Resources/Texture Files/Gold/gold_normal.png",				// Normal
			L"Resources/Texture Files/Gold/gold_ambientOcclusion.png")));	// AO
	AddEntity(gold_sphere);

	Entity* test_sphere_00 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-4.4f, 4.4f, 0),		// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_00->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			1.0f,		// Metalness
			0.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_00);

	Entity* test_sphere_01 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-2.2f, 4.4f, 0),		// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_01->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			1.0f,		// Metalness
			0.25f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_01);

	Entity* test_sphere_02 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 4.4f, 0),			// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_02->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
			std::make_shared<Material>(
			1.0f,		// Metalness
			0.5f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_02);

	Entity* test_sphere_03 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(2.2f, 4.4f, 0),		// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_03->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
			std::make_shared<Material>(
			1.0f,		// Metalness
			0.75f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_03);

	Entity* test_sphere_04 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(4.4f, 4.4f, 0),		// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_04->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			1.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_04);

	Entity* test_sphere_10 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-4.4f, 2.2f, 0),		// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_10->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			0.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_10);

	Entity* test_sphere_11 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-2.2f, 2.2f, 0),		// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_11->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			0.25f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_11);

	Entity* test_sphere_12 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 2.2f, 0),			// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_12->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			0.5f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_12);

	Entity* test_sphere_13 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(2.2f, 2.2f, 0),		// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_13->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			0.75f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_13);

	Entity* test_sphere_14 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(4.4f, 2.2f, 0),		// Position
			XMFLOAT3(0, 0, 0),				// Rotation
			XMFLOAT3(1.75, 1.75, 1.75)));	// Scale
	test_sphere_14->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	AddEntity(test_sphere_14);
}

DemoScene::~DemoScene()
{
}
