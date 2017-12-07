#include "Framework.h"

using namespace DirectX;

Framework::Framework(HINSTANCE p_hinstance)
	: D3D11Renderer(
		p_hinstance,
		"Ori Engine",		// Window title
		1750,				// Window client width
		750)				// Window client height
{
	//
}

Framework::~Framework()
{
}

void Framework::Initialize()
{
	// Systems
	mup_scene_manager = std::make_unique<SceneSystem>();
	mup_camera_system = std::make_unique<CameraSystem>();
	mup_graphics_system = std::make_unique<GraphicsSystem>(mcp_device.Get(), mcp_context.Get(), mcp_swapchain.Get(), mcp_backbuffer_rtv.Get(), m_width, m_height);
	mup_input_system = std::make_unique<InputSystem>(*mup_scene_manager.get(), *mup_camera_system.get(), &m_hwnd);
	
	// Tools
	ImageBasedLightingBaker * imageBasedLightingBaker = mup_graphics_system->GetImageBasedLightingBaker();
	ShadowRenderer* shadowRenderer = mup_graphics_system->GetShadowRenderer();

	// Scenes
	Scene& current_scene = mup_scene_manager->AddEmptyScene();	// move to a derived demo scene, requires moving lighting baking to a run-time check

	SkyBox * skybox = new SkyBox();
	skybox->LoadEnvMap(*mcp_device.Get(), L"Resources/Cube Maps/Humus/Stairs.dds");
	skybox->LoadIrradianceMap(*mcp_device.Get(), L"Resources/Cube Maps/Humus/Stairs_Irradiance.dds");
	imageBasedLightingBaker->GeneratePfem(skybox);
	imageBasedLightingBaker->GenerateBrdfLut(skybox);
	current_scene.SetCurrentSkyBox(skybox);

	Entity* camera = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 2, -8),		// Position
			XMFLOAT3(0, 0, 1))		// Rotation
	);
	camera->AddComponent(std::make_unique<CameraComponent>(
		camera->GetTransformComponent(),
		0.25f * XM_PI,						// FOV
		(float)m_width / m_height,			// Aspect Ratio
		0.1f,								// Near Clip
		100.0f));							// Far Clip
	//current_scene.AddCamera();
	current_scene.SetCurrentCamera(camera);

	Entity* sun = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 5, 0),			// position
			XMFLOAT3(0, -0.00001f, 1))	// rotation
	);
	sun->AddComponent(std::make_unique<LightComponent>(
		std::make_shared<DirectionalLight>(
			XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f),		// Ambient color
			XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f))));	// Diffuse color
	shadowRenderer->EnableShadowing(*sun);
	shadowRenderer->EnableCascadedShadowing(*sun);
	//current_scene.AddLight();
	current_scene.mSun = sun;

	Entity* emitter = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 2.2f, -2.2f))	// position
	);
	emitter->AddComponent(std::make_unique<ParticleEmitterComponent>(
		emitter->GetTransformComponent(),
		mcp_device.Get())
	);
	current_scene.AddEntity(emitter);

	//std::shared_ptr<Mesh> cube_mesh = std::make_shared<Mesh>(mcp_device.Get(), "Resources/Mesh Files/cube.obj");
	std::shared_ptr<Mesh> sphere_mesh = std::make_shared<Mesh>(mcp_device.Get(), "Resources/Mesh Files/sphere.obj");

	Entity* wood_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-4.4f, 0, 0),		// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	wood_sphere->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(mcp_device.Get(), mcp_context.Get(),
			L"Resources/Texture Files/OakFloor/oakfloor_basecolor.png",		// Diffuse
			L"Resources/Texture Files/Blanks/Full_0.png",					// Metallic
			L"Resources/Texture Files/OakFloor/oakfloor_roughness.png",		// Roughness
			L"Resources/Texture Files/OakFloor/oakfloor_normal.png",		// Normal
			L"Resources/Texture Files/OakFloor/oakfloor_AO.png")));			// AO
	current_scene.AddEntity(wood_sphere);

	Entity* rock_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-2.2f, 0, 0),		// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	rock_sphere->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(mcp_device.Get(), mcp_context.Get(),
			L"Resources/Texture Files/Rock/TexturesCom_Rock_1024_albedo.tif",		// Diffuse
			L"Resources/Texture Files/Blanks/Full_0.png",							// Metallic
			L"Resources/Texture Files/Rock/TexturesCom_Rock_1024_roughness.tif",	// Roughness
			L"Resources/Texture Files/Rock/TexturesCom_Rock_1024_normal.tif",		// Normal
			L"Resources/Texture Files/Rock/TexturesCom_Rock_1024_ao.tif")));		// AO
	current_scene.AddEntity(rock_sphere);

	Entity* brick_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 0, 0),			// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	brick_sphere->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(mcp_device.Get(), mcp_context.Get(),
			L"Resources/Texture Files/Bricks/brick_wall_001_baseColor.png",				// Diffuse
			L"Resources/Texture Files/Bricks/brick_wall_001_metallic.png",				// Metallic
			L"Resources/Texture Files/Bricks/brick_wall_001_roughness.png",				// Roughness
			L"Resources/Texture Files/Bricks/brick_wall_001_normal.png",				// Normal
			L"Resources/Texture Files/Bricks/brick_wall_001_ambientOcclusion.png")));	// AO
	current_scene.AddEntity(brick_sphere);

	Entity* rust_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(2.2f, 0, 0),		// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	rust_sphere->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(mcp_device.Get(), mcp_context.Get(),
			L"Resources/Texture Files/RustedIron/rustediron2_basecolor.png",			// Diffuse
			L"Resources/Texture Files/RustedIron/rustediron2_metallic.png",				// Metallic
			L"Resources/Texture Files/RustedIron/rustediron2_roughness.png",			// Roughness
			L"Resources/Texture Files/RustedIron/rustediron2_normal.png",				// Normal
			L"Resources/Texture Files/RustedIron/rustediron2_ambientOcclusion.png")));	// AO
	current_scene.AddEntity(rust_sphere);

	Entity* gold_sphere = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(4.4f, 0, 0),		// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	gold_sphere->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(mcp_device.Get(), mcp_context.Get(),
			L"Resources/Texture Files/Gold/gold_baseColor.png",				// Diffuse
			L"Resources/Texture Files/Gold/gold_metallic.png",				// Metallic
			L"Resources/Texture Files/Gold/gold_roughness.png",				// Roughness
			L"Resources/Texture Files/Gold/gold_normal.png",				// Normal
			L"Resources/Texture Files/Gold/gold_ambientOcclusion.png")));	// AO
	current_scene.AddEntity(gold_sphere);

	/*	=====================================================================================================
			Test Materials
		=====================================================================================================	*/

	/*
	Entity* shadow_floor = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, -2.0f, 0);			// Position
			XMFLOAT3(0, 0, 0);				// Rotation
			XMFLOAT3(20.0f, 0.1f, 20.0f))	// Scale
	);
	shadow_floor->AddComponent(std::make_unique<DrawComponent>(
		cube_mesh,
		std::make_shared<Material>(
		1.0f,		// Metalness
		1.0f,		// Roughness
		1.0f)));	// Opacity
	current_scene.AddEntity(shadow_floor);
	*/

	Entity* test_sphere_00 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-4.4f, 4.4f, 0),	// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_00->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			1.0f,		// Metalness
			0.0f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_00);

	Entity* test_sphere_01 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-2.2f, 4.4f, 0),	// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_01->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			1.0f,		// Metalness
			0.25f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_01);

	Entity* test_sphere_02 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 4.4f, 0),		// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_02->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			1.0f,		// Metalness
			0.5f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_02);

	Entity* test_sphere_03 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(2.2f, 4.4f, 0),	// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_03->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			1.0f,		// Metalness
			0.75f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_03);

	Entity* test_sphere_04 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(4.4f, 4.4f, 0),	// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_04->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			1.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_04);

	Entity* test_sphere_10 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-4.4f, 2.2f, 0),	// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_10->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			0.0f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_10);

	Entity* test_sphere_11 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(-2.2f, 2.2f, 0),	// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_11->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			0.25f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_11);

	Entity* test_sphere_12 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(0, 2.2f, 0),		// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_12->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			0.5f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_12);

	Entity* test_sphere_13 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(2.2f, 2.2f, 0),	// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_13->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			0.75f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_13);

	Entity* test_sphere_14 = new Entity(
		std::make_unique<TransformComponent>(
			XMFLOAT3(4.4f, 2.2f, 0),	// Position
			XMFLOAT3(0, 0, 0),			// Rotation
			XMFLOAT3(1.75, 1.75, 1.75))	// Scale
	);
	test_sphere_14->AddComponent(std::make_unique<DrawComponent>(
		sphere_mesh,
		std::make_shared<Material>(
			0.0f,		// Metalness
			1.0f,		// Roughness
			1.0f)));	// Opacity
	current_scene.AddEntity(test_sphere_14);
}

void Framework::Update(float p_delta_time, float p_total_time)
{
	// passing events instead of reference control in input system?
	mup_input_system->Update(p_delta_time);
	mup_graphics_system->Update(mup_scene_manager->GetCurrentScene(), p_delta_time);
}

void Framework::Draw(float p_delta_time, float p_total_time)
{
	mup_graphics_system->Draw(mup_scene_manager->GetCurrentScene(), p_delta_time);
}