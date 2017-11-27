#include "Scene.h"

Scene::Scene()
{
}

Scene::~Scene()
{
	delete mCurrentCamera;
	delete mCurrentSkyBox;
	delete mSun;
}

void Scene::AddEntity(Entity* pEntity)
{
	mEntities.push_back(std::move(std::unique_ptr<Entity>(pEntity)));
}

std::vector<std::unique_ptr<Entity>>& Scene::GetEntities()
{
	return mEntities;
}

void Scene::SetCurrentCamera(Entity* pCamera)
{
	mCurrentCamera = pCamera;
}

Entity* Scene::GetCurrentCamera()
{
	return mCurrentCamera;
}

void Scene::SetCurrentSkyBox(SkyBox * pSkyBox)
{
	mCurrentSkyBox = pSkyBox;
}

SkyBox* Scene::GetCurrentSkyBox()
{
	return mCurrentSkyBox;
}
