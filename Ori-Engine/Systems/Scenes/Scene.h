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
	std::vector<std::unique_ptr<Entity>> & GetEntities();
	//std::vector<std::unique_ptr<Entity>>& GetLightVector() const;
	void SetCurrentCamera(Entity* pCamera);
	Entity* GetCurrentCamera();
	void SetCurrentSkyBox(SkyBox * pSkyBox);
	SkyBox* GetCurrentSkyBox();

	Entity* mSun;
	
private:

	std::vector<std::unique_ptr<Entity>> mEntities;
	//std::vector<std::unique_ptr<Entity>> mLightVector;
	Entity* mCurrentCamera;
	SkyBox* mCurrentSkyBox;
};

