#include "Scene.h"

Scene::Scene()
{
}

Scene::~Scene()
{
	delete mCurrentCamera;
	delete mCurrentSkyBox;
}

void Scene::AddEntity(Entity* pEntity)
{
	mEntities.push_back(std::move(std::unique_ptr<Entity>(pEntity)));
}

const std::vector<std::unique_ptr<Entity>>& Scene::GetEntities() const
{
	return mEntities;
}

void Scene::AddLight(Entity* p_light)
{
	m_lights.push_back(std::move(std::unique_ptr<Entity>(p_light)));
}

const std::vector<std::unique_ptr<Entity>>& Scene::GetLights() const
{
	return m_lights;
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
