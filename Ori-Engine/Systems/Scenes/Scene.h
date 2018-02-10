#pragma once

#include <memory>
#include <vector>

#include "Entity.h"
#include "SkyBox.h"

class Scene
{
public:
	Scene();
	~Scene();

	void AddEntity(Entity* pEntity);
	const std::vector<std::unique_ptr<Entity>>& GetEntities() const;
	void AddLight(Entity* p_light);
	const std::vector<std::unique_ptr<Entity>>& GetLights() const;
	void SetCurrentCamera(Entity* pCamera);
	Entity* GetCurrentCamera();
	void SetCurrentSkyBox(SkyBox * pSkyBox);
	SkyBox* GetCurrentSkyBox();

	bool is_tone_mapped = false;
	bool enable_ssao = false;
	
private:
	std::vector<std::unique_ptr<Entity>> mEntities;
	std::vector<std::unique_ptr<Entity>> m_lights;
	Entity* mCurrentCamera;
	SkyBox* mCurrentSkyBox;
};

