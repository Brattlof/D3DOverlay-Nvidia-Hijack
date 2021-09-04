#pragma once
#include <functional>
#include <thread>

#include <dwmapi.h>
#pragma comment (lib, "dwmapi.lib")

#include <d3d9.h>
#pragma comment (lib, "d3d9.lib")

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "XorStr.h"

class D3DOverlay
{
private:
	static std::function<void(int, int)> m_UserRender;
	static HWND m_Window;
	static HWND m_TargetWindow;
	static int m_WindowWidth;
	static int m_WindowHeight;

	static LPDIRECT3D9 m_D3D;
	static LPDIRECT3DDEVICE9 m_D3Device;
	static D3DPRESENT_PARAMETERS m_D3Parameters;

	static bool CreateOverlay();
	static bool CreateDeviceD3D();

public:
	static bool Init(const HWND TargetWindow);
	static bool Update(bool MenuOpen);
	static void SetUserRender(const std::function<void(int, int)> UserRender);
	static void CleanupOverlay();

	static void DrawString(float x, float y, ImU32 Color, std::string String);
	static void DrawRect(float x, float y, float w, float h, ImU32 Color, float Thickness);
	static void DrawFilledRect(float x, float y, float w, float h, ImU32 Color);
	static void DrawCircle(float x, float y, float Radius, ImU32 Color, int Segments);
};