#include "SceneSystem.h"

SceneSystem::SceneSystem()
{
}

SceneSystem::~SceneSystem()
{
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
