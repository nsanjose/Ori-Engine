#include "D3D11Renderer.h"

#include <iostream>
#include <sstream>
#include <WindowsX.h>

D3D11Renderer::D3D11Renderer(HINSTANCE p_hinstance, char* p_title_bar_text, unsigned int p_window_width, unsigned int p_window_height)
{
	m_hinstance = p_hinstance;
	m_title_bar_text = p_title_bar_text;
	m_width = p_window_width;
	m_height = p_window_height;
}

D3D11Renderer::~D3D11Renderer()
{
	mcp_context->ClearState();
	mcp_context->Flush();

	if (m_debug_interface != nullptr)
	{
		m_debug_interface->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	}
}

HRESULT D3D11Renderer::InitializeWindow()
{
	WNDCLASS window_class		= {};
	window_class.style			= CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc	= InputSystem::WindowProc;		// Handler of window messages
	window_class.cbClsExtra		= 0;
	window_class.cbWndExtra		= 0;
	window_class.hInstance		= m_hinstance;
	window_class.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	window_class.hCursor		= LoadCursor(NULL, IDC_ARROW);
	window_class.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	window_class.lpszMenuName	= NULL;
	window_class.lpszClassName	= "MyWindowClass";
	RegisterClass(&window_class);

	DWORD window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;	// No maximize button

	// Adjust window size so client matches specified dimensions
	RECT client_rect;
	SetRect(&client_rect, 0, 0, m_width, m_height);
	AdjustWindowRect(&client_rect, window_style, false);

	// Center window to the screen
	RECT screen_rect;
	GetClientRect(GetDesktopWindow(), &screen_rect);
	int centered_window_left = (screen_rect.right / 2) - (client_rect.right / 2);
	int centered_window_top = (screen_rect.bottom / 2) - (client_rect.bottom / 2);
	int adjusted_window_width = client_rect.right - client_rect.left;
	int adjusted_window_height = client_rect.bottom - client_rect.top;

	// Create and verify window
	m_hwnd = CreateWindow(
		window_class.lpszClassName,
		m_title_bar_text.c_str(),
		window_style,	
		centered_window_left,
		centered_window_top,
		adjusted_window_width,
		adjusted_window_height,
		0,				// No parent window
		0,				// No menu
		m_hinstance,
		0);				// No multiple windows

	if (m_hwnd == NULL)
	{
		DWORD error = GetLastError();
		return HRESULT_FROM_WIN32(error);
	}

	// Display window
	ShowWindow(m_hwnd, SW_SHOW);

	return S_OK;
}

HRESULT D3D11Renderer::InitializeDirectX()
{
	HRESULT hr = S_OK;
	unsigned int mcp_device_flags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	mcp_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC swapchain_desc					= {};
	swapchain_desc.BufferDesc.Width						= m_width;
	swapchain_desc.BufferDesc.Height					= m_height;
	swapchain_desc.BufferDesc.RefreshRate.Numerator		= 60;
	swapchain_desc.BufferDesc.RefreshRate.Denominator	= 1;
	swapchain_desc.BufferDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	swapchain_desc.BufferDesc.ScanlineOrdering			= DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapchain_desc.BufferDesc.Scaling					= DXGI_MODE_SCALING_UNSPECIFIED;
	swapchain_desc.SampleDesc.Count						= 1;
	swapchain_desc.SampleDesc.Quality					= 0;
	swapchain_desc.BufferCount							= 1;
	swapchain_desc.BufferUsage							= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain_desc.OutputWindow							= m_hwnd;
	swapchain_desc.Windowed								= true;
	swapchain_desc.SwapEffect							= DXGI_SWAP_EFFECT_DISCARD;
	swapchain_desc.Flags								= 0;

	hr = D3D11CreateDeviceAndSwapChain(
		0,							// Default video adapter
		D3D_DRIVER_TYPE_HARDWARE,	// Driver type for video adapter
		0,
		mcp_device_flags,
		0,							// Array of feature levels to try and fallback
		0,							// Number of elements in that array
		D3D11_SDK_VERSION,			// SDK Version
		&swapchain_desc,
		&mcp_swapchain,
		&mcp_device,
		&m_directx_feature_level,
		&mcp_context);
	if (FAILED(hr)) return hr;

	// Get rtv from the swap chain's back buffer.
	ID3D11Texture2D* backbuffer_texture;
	hr = mcp_swapchain->GetBuffer(0,	__uuidof(ID3D11Texture2D), (void**)&backbuffer_texture);
	if (FAILED(hr)) return hr;
	hr = mcp_device->CreateRenderTargetView(backbuffer_texture, 0, &mcp_backbuffer_rtv);
	if (FAILED(hr)) return hr;
	backbuffer_texture->Release();

	return S_OK;
}

void D3D11Renderer::OnResize()
{
	// Resize back buffers and their rtvs to new window dimensions
	mcp_swapchain->ResizeBuffers(1, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 0);

	ID3D11Texture2D* backbuffer_texture;
	mcp_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backbuffer_texture));
	mcp_device->CreateRenderTargetView(backbuffer_texture, 0, &mcp_backbuffer_rtv);
	std::string back_buffer_rtv_name("Back Buffer RTV");
	mcp_backbuffer_rtv->SetPrivateData(WKPDID_D3DDebugObjectName, back_buffer_rtv_name.size(), back_buffer_rtv_name.c_str());
	backbuffer_texture->Release();
}

HRESULT D3D11Renderer::Run()
{
	HRESULT hr = S_OK;

	// Initialize Self
#if defined(DEBUG) || defined(_DEBUG)
	CreateConsoleWindow();
	std::cout << "Created console window for debugging.\n";
#endif

	hr = InitializeWindow();
	if (FAILED(hr)) return hr;
#if defined(DEBUG) || defined(_DEBUG)
	std::cout << "Created window.\n";
#endif

	hr = InitializeDirectX();
	if (FAILED(hr)) return hr;
#if defined(DEBUG) || defined(_DEBUG)
	std::cout << "Initialized DirectX device, context, swapchain, and backbuffer_rtv.\n";
#endif

#if defined(DEBUG) || defined(_DEBUG)
	hr = mcp_device.Get()->QueryInterface(IID_PPV_ARGS(&m_debug_interface));
	if (FAILED(hr)) return hr;
	std::cout << "Enabled a debugging interface for DirectX.\n";
#endif

	InitializeTime();
#if defined(DEBUG) || defined(_DEBUG)
	std::cout << "Initialized time keeping.\n";
#endif

	// Initialize Framework
	Initialize();
#if defined(DEBUG) || defined(_DEBUG)
	std::cout << "Initialized framework.\n";
#endif

	// Application Loop
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);	// Window messages are sent to InputSystem::WindowProc
		}
		else
		{
			UpdateTimer();
			UpdateTitleBar();

			Update(m_delta_time, m_total_time);
			Draw(m_delta_time, m_total_time);
		}
	}

	// Exit
	return msg.wParam;
}

void D3D11Renderer::InitializeTime()
{
	LARGE_INTEGER time_update_frequency;
	QueryPerformanceFrequency(&time_update_frequency);
	m_time_update_rate = 1.0 / (double)time_update_frequency.QuadPart;

	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	m_start_time = current_time.QuadPart;
	m_current_time = current_time.QuadPart;
	m_previous_time = current_time.QuadPart;
	m_total_time = 0.0f;

	m_current_fps = 0;
	m_previous_fps_update_total_time = 0.0f;
}

void D3D11Renderer::UpdateTimer()
{
	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	m_current_time = current_time.QuadPart;

	m_delta_time = max((float)((m_current_time - m_previous_time) * m_time_update_rate), 0.0f);	// can go negative in power save mode or thread switching?
	m_total_time = (float)((m_current_time - m_start_time) * m_time_update_rate);
	m_previous_time = m_current_time;
}

void D3D11Renderer::UpdateTitleBar()
{
	m_current_fps++;

	// Only update title bar once per second
	float time_since_update = m_total_time - m_previous_fps_update_total_time;
	if (time_since_update < 1.0f)
	{
		return;
	}

	float frame_time = 1000.0f / (float)m_current_fps;

	std::ostringstream output;
	output.precision(6);
	output	<< m_title_bar_text <<
			"    FPS: "			<< m_current_fps <<
			"    Frame Time: "	<< frame_time << "ms";

	
	// Display
	SetWindowText(m_hwnd, output.str().c_str());
	
	// Reset
	m_current_fps = 0;
	m_previous_fps_update_total_time = m_total_time;
}

void D3D11Renderer::CreateConsoleWindow() // Using default command prompt buffer and window dimensions (80x300, 80x25)
{
	AllocConsole();

	// Set dimensions of console screen buffer (char row/col)
	CONSOLE_SCREEN_BUFFER_INFO console_buffer_info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_buffer_info);
	console_buffer_info.dwSize.X = 80;
	console_buffer_info.dwSize.Y = 300;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), console_buffer_info.dwSize);

	// Set dimensions of console window
	SMALL_RECT console_window_rect;
	console_window_rect.Left = 0;
	console_window_rect.Top = 0;
	console_window_rect.Right = 80;
	console_window_rect.Bottom = 25;
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &console_window_rect);

	// Position on top left of screen
	HWND console_window_handle = GetConsoleWindow();
	SetWindowPos(console_window_handle, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	FILE *stream;
	freopen_s(&stream, "CONIN$", "r", stdin);
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);
}