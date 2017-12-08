#pragma once

#include <memory>
#include <vector>

#include "Scene.h"

class SceneSystem
{
public:
	SceneSystem();
	~SceneSystem();

	void AddScene(std::unique_ptr<Scene> pup_scene);
	Scene& AddEmptyScene();
	Scene* GetCurrentScene();

	void UpdateMatrices();

private:
	std::vector<std::unique_ptr<Scene>> mup_scenes;
	Scene* mp_current_scene;
};

