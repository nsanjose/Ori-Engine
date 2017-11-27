#include "InputSystem.h"

#include "..\Scenes\Scene.h"
#include "..\Scenes\Entity.h"

InputSystem* InputSystem::input_system_instance = 0;
LRESULT InputSystem::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_DESTROY:
			PostQuitMessage(0); // override?
			return 0;

			// Prevent beeping when we "alt-enter"
		case WM_MENUCHAR:
			return MAKELRESULT(0, MNC_CLOSE);

			/* Resizing is currently disabled.
			case WM_SIZE:
			Update Framework client dimensions.
			Update DirectX resources
			return 0;
			*/

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			input_system_instance->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			input_system_instance->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WM_MOUSEMOVE:
			input_system_instance->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WM_MOUSEWHEEL:
			input_system_instance->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

			// Only on key down, not while pressed
		case WM_KEYDOWN:
			input_system_instance->OnKeyDown(wParam);
			return 0;
	}

	// Default handling for unspecified messages
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

InputSystem::InputSystem(SceneSystem & pSceneSystem, CameraSystem & pCameraSystem, HWND * pHWnd)
	: mr_scene_system(pSceneSystem), mr_camera_system(pCameraSystem)
{
	hWnd = pHWnd;
	input_system_instance = this;
}

InputSystem::~InputSystem()
{
}

void InputSystem::Update(float deltaTime)
{
	// Close application on escape
	if (GetAsyncKeyState(VK_ESCAPE))
		PostQuitMessage(0);

	Scene* current_scene = mr_scene_system.GetCurrentScene();
	if (current_scene != nullptr)
	{
		Entity* current_camera = current_scene->GetCurrentCamera();
		if (current_camera != nullptr)
		{
			// Camera Strafe
			if (GetAsyncKeyState('W') & 0x8000) { mr_camera_system.MoveRelative(*current_camera, 0, 0, 1 * deltaTime); }	// forward
			if (GetAsyncKeyState('S') & 0x8000) { mr_camera_system.MoveRelative(*current_camera, 0, 0, -1 * deltaTime); }	// back
			if (GetAsyncKeyState('A') & 0x8000) { mr_camera_system.MoveRelative(*current_camera, -1 * deltaTime, 0, 0); }	// left
			if (GetAsyncKeyState('D') & 0x8000) { mr_camera_system.MoveRelative(*current_camera, 1 * deltaTime, 0, 0); }	// right
			if (GetAsyncKeyState(' ') & 0x8000) { mr_camera_system.MoveRelative(*current_camera, 0, 1 * deltaTime, 0); }	// up
			if (GetAsyncKeyState('X') & 0x8000) { mr_camera_system.MoveRelative(*current_camera, 0, -1 * deltaTime, 0); }	// down
		}
	}
}

void InputSystem::OnMouseDown(WPARAM buttonState, int x, int y)
{
	previous_mouse_position.x = x;
	previous_mouse_position.y = y;

	// Track mouse outside of window
	SetCapture(*hWnd);
}

void InputSystem::OnMouseUp(WPARAM buttonState, int x, int y)
{
	// Stop tracking mouse
	ReleaseCapture();
}

void InputSystem::OnMouseMove(WPARAM buttonState, int p_mouse_position_x, int p_mouse_position_y)
{
	Scene* current_scene = mr_scene_system.GetCurrentScene();
	if (current_scene != nullptr)
	{
		Entity* current_camera = current_scene->GetCurrentCamera();
		if (current_camera != nullptr)
		{
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
			{
				// Camera Rotation
				float mouse_position_x_difference = p_mouse_position_x - (float)previous_mouse_position.x;
				float mouse_position_y_difference = p_mouse_position_y - (float)previous_mouse_position.y;
				mr_camera_system.Rotate(*current_camera, mouse_position_y_difference, mouse_position_x_difference);
			}
		}
	}

	previous_mouse_position.x = p_mouse_position_x;
	previous_mouse_position.y = p_mouse_position_y;
}

void InputSystem::OnMouseWheel(float wheelDelta, int x, int y)
{
	mr_camera_system.AdjustSpeed(wheelDelta);
}

void InputSystem::OnKeyDown(WPARAM keyState)
{
	//
}