#include "SceneSystem.h"

SceneSystem::SceneSystem()
{
}

SceneSystem::~SceneSystem()
{
}

void SceneSystem::AddScene(Scene* pp_scene)
{
	mup_scenes.push_back(std::move(std::unique_ptr<Scene>(pp_scene)));
	if (mup_scenes.size() == 1)
	{
		mp_current_scene = mup_scenes.back().get();
	}
}

void SceneSystem::AddScene(std::unique_ptr<Scene> pup_scene)
{
	mup_scenes.push_back(std::move(pup_scene));
	if (mup_scenes.size() == 1)
	{
		mp_current_scene = mup_scenes.back().get();
	}
}

Scene& SceneSystem::AddEmptyScene()
{
	mup_scenes.push_back(std::move(std::make_unique<Scene>()));
	if (mup_scenes.size() == 1)
	{
		mp_current_scene = mup_scenes.back().get();
	}
	return *mup_scenes.back().get();
}

Scene* SceneSystem::GetCurrentScene()
{
	return mp_current_scene;
}

void SceneSystem::UpdateMatrices()
{
	// Update all world matrices
	for (int i = 0; i < mp_current_scene->GetEntities().size(); i++)
	{
		mp_current_scene->GetEntities()[i].get()->GetTransformComponent().UpdateWorldMatrix();
	}
	mp_current_scene->GetCurrentCamera()->GetTransformComponent().UpdateWorldMatrix();
}
