#pragma once

#pragma comment(lib, "d3d11.lib")

#include <d3d11.h>
#include <memory>
#include <string>
#include <Windows.h>
#include <wrl.h>

#include "..\Systems\Input\InputSystem.h"

class D3D11Renderer
{
public:
	D3D11Renderer(HINSTANCE p_hinstance, char* p_title_bar_text, unsigned int p_window_width, unsigned int p_window_height);
	~D3D11Renderer();

	HRESULT InitializeWindow();
	HRESULT InitializeDirectX();
	HRESULT Run();

	virtual void Initialize() = 0;
	virtual void OnResize();
	virtual void Update(float m_delta_time, float m_total_time) = 0;
	virtual void Draw(float m_delta_time, float m_total_time) = 0;

	HINSTANCE		m_hinstance;		// Handle to application
	// Window Resources
	HWND			m_hwnd;				// Hanlde to window
	std::string		m_title_bar_text;	// Window title bar text
	unsigned int	m_width;			// Window client width
	unsigned int	m_height;			// Window client height

	// Base DirectX Resources
	D3D_FEATURE_LEVEL m_directx_feature_level;
	Microsoft::WRL::ComPtr<ID3D11Device>			mcp_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		mcp_context;
	Microsoft::WRL::ComPtr<IDXGISwapChain>			mcp_swapchain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	mcp_backbuffer_rtv;

	// For Debugging
	ID3D11Debug* m_debug_interface;
	void CreateConsoleWindow();

private:
	// Time Resources
	double m_time_update_rate;
	__int64 m_start_time;
	__int64 m_current_time;
	__int64 m_previous_time;
	float m_delta_time;
	float m_total_time;

	// Frames Per Second Stats
	int m_current_fps;
	float m_previous_fps_update_total_time;

	void InitializeTime();
	void UpdateTimer();
	void UpdateTitleBar();
};

