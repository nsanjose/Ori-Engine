#pragma once

#if defined(DEBUG) || defined(_DEBUG)
	#include "vld.h"
#endif

#include <Windows.h>

#include "Framework.h"

int WINAPI WinMain(HINSTANCE p_hinstance, HINSTANCE p_previous_hinstance, LPSTR p_command_line, int p_window_state)
{
	Framework app(p_hinstance);
	return app.Run();
}
