#pragma once

#include <memory>
#include <Windows.h>
#include <WindowsX.h>

#include "..\Cameras\CameraSystem.h"
#include "..\Scenes\SceneSystem.h"

class InputSystem
{
public:
	InputSystem(SceneSystem& pSceneSystem, CameraSystem& pCameraSystem, HWND* pHWnd);
	~InputSystem();

	static InputSystem* input_system_instance;
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnMouseDown(WPARAM buttonState, int x, int y);
	void OnMouseUp(WPARAM buttonState, int x, int y);
	void OnMouseMove(WPARAM buttonState, int x, int y);
	void OnMouseWheel(float wheelDelta, int x, int y);
	void OnKeyDown(WPARAM keyPressed);

	void Update(float deltaTime);

private:
	// References
	SceneSystem & mr_scene_system;
	CameraSystem & mr_camera_system;
	HWND* hWnd;

	// Resources
	POINT previous_mouse_position;
};

