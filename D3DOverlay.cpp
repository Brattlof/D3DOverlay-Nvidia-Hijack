#include "D3DOverlay.hpp"

#define Log(x) printf(_("[D3LOG] %s\n"), x)

std::function<void(int, int)> D3DOverlay::m_UserRender;

HWND D3DOverlay::m_Window;
HWND D3DOverlay::m_TargetWindow;
int D3DOverlay::m_WindowWidth;
int D3DOverlay::m_WindowHeight;

LPDIRECT3D9 D3DOverlay::m_D3D;
LPDIRECT3DDEVICE9 D3DOverlay::m_D3Device;
D3DPRESENT_PARAMETERS D3DOverlay::m_D3Parameters;

bool D3DOverlay::Init(const HWND TargetWindow)
{
	if (!TargetWindow || !IsWindow(TargetWindow))
	{
		CleanupOverlay();
		Log(_("Invalid target window"));
		return false;
	}

	m_TargetWindow = TargetWindow;

	if (!CreateOverlay())
	{
		CleanupOverlay();
		Log(_("Failed to create overlay"));
		return false;
	}

	if (!CreateDeviceD3D())
	{
		CleanupOverlay();
		Log(_("Failed creating D3D device"));
		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(m_Window);
	ImGui_ImplDX9_Init(m_D3Device);

	Log(_("Created overlay successfully"));

	return true;
}

bool D3DOverlay::CreateOverlay()
{
	m_Window = FindWindowA(_("CEF-OSC-WIDGET"), _("NVIDIA GeForce Overlay"));
	if (!m_Window)
		return false;

	SetWindowLongPtr(m_Window, -20, (LONG_PTR)((int)GetWindowLong(m_Window, -20) | 0x20));

	MARGINS Margin = {-1};
	DwmExtendFrameIntoClientArea(m_Window, &Margin);
	SetLayeredWindowAttributes(m_Window, 0x000000, 0xFF, 0x02);

	SetWindowPos(m_Window, HWND_TOPMOST, 0, 0, 0, 0, 0x0002 | 0x0001);

	return true;
}

bool D3DOverlay::CreateDeviceD3D()
{
	if ((m_D3D = Direct3DCreate9(D3D_SDK_VERSION)) == 0)
		return false;

	ZeroMemory(&m_D3Parameters, sizeof(m_D3Parameters));
	m_D3Parameters.Windowed = true;
	m_D3Parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_D3Parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
	m_D3Parameters.EnableAutoDepthStencil = true;
	m_D3Parameters.AutoDepthStencilFormat = D3DFMT_D16;
	m_D3Parameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (m_D3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_Window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_D3Parameters, &m_D3Device) < 0)
		return false;

	return true;
}

bool D3DOverlay::Update(bool MenuOpen)
{
	if (m_Window)
	{
		MSG Msg{ 0 };

		if (::PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&Msg);
			::DispatchMessage(&Msg);
		}

		if (!m_TargetWindow || !IsWindow(m_TargetWindow))
			return false;

		if (MenuOpen)
		{
			SetWindowLong(m_Window, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOOLWINDOW);

			auto& io = ImGui::GetIO();
			io.MouseDown[0] = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
			io.MouseDown[1] = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
		}
		else
		{
			SetWindowLong(m_Window, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
		}

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		m_UserRender(m_WindowWidth, m_WindowHeight);

		DWORD color_write = 0UL, srgb_write = 0UL;
		IDirect3DVertexDeclaration9* vertex_declaration = nullptr;
		IDirect3DVertexShader9* vertex_shader = nullptr;
		m_D3Device->GetRenderState(D3DRS_COLORWRITEENABLE, &color_write);
		m_D3Device->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgb_write);
		m_D3Device->GetVertexDeclaration(&vertex_declaration);
		m_D3Device->GetVertexShader(&vertex_shader);

		m_D3Device->SetVertexShader(nullptr);
		m_D3Device->SetPixelShader(nullptr);
		m_D3Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		m_D3Device->SetRenderState(D3DRS_LIGHTING, false);
		m_D3Device->SetRenderState(D3DRS_FOGENABLE, false);
		m_D3Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		m_D3Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

		m_D3Device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		m_D3Device->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
		m_D3Device->SetRenderState(D3DRS_ZWRITEENABLE, false);
		m_D3Device->SetRenderState(D3DRS_STENCILENABLE, false);

		m_D3Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, false);
		m_D3Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, false);

		m_D3Device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		m_D3Device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
		m_D3Device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true);
		m_D3Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_D3Device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA);
		m_D3Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		m_D3Device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);

		m_D3Device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
		m_D3Device->SetRenderState(D3DRS_SRGBWRITEENABLE, false);

		ImGui::EndFrame();

		m_D3Device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 0), 1.f, 0);

		//	If it's not D3D_OK
		if (m_D3Device->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			m_D3Device->EndScene();
		}

		m_D3Device->Present(NULL, NULL, NULL, NULL);

		m_D3Device->SetRenderState(D3DRS_COLORWRITEENABLE, color_write);
		m_D3Device->SetRenderState(D3DRS_SRGBWRITEENABLE, srgb_write);

		return Msg.message != WM_QUIT;
	}

	return false;
}

void D3DOverlay::SetUserRender(const std::function<void(int, int)> UserRender)
{
	m_UserRender = UserRender;
}

void D3DOverlay::CleanupOverlay()
{
	if (m_D3Device)
	{
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		m_D3Device->Release();
	}

	if (m_D3D)
		m_D3D->Release();

	if (m_Window)
		DestroyWindow(m_Window);
}

void D3DOverlay::DrawString(float x, float y, ImU32 Color, std::string String)
{
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y), Color, String.data());
}

void D3DOverlay::DrawRect(float x, float y, float w, float h, ImU32 Color, float Thickness)
{
	ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), Color, 0, 0, Thickness);
}

void D3DOverlay::DrawFilledRect(float x, float y, float w, float h, ImU32 Color)
{
	ImGui::GetOverlayDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), Color);
}

void D3DOverlay::DrawCircle(float x, float y, float Radius, ImU32 Color, int Segments)
{
	ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(x, y), Radius, Color, Segments);
}