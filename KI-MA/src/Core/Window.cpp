#include "Window.h"

#include "ImGui/backends/imgui_impl_win32.h"

#include <Windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


namespace Core {
	const wchar_t ClassName[] = L"WINDOW";

	LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		// Zugriff auf die Instanz der Window-Klasse über das Fensterhandle
		Window* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

		// Die Nachricht an ImGui weiterleiten
		ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);

		switch (uMsg) {
		case WM_SIZE:
		{
			if (window) {
				window->m_Size.x = LOWORD(lParam);
				window->m_Size.y = HIWORD(lParam);
			}
			break;
		}
		};

		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}

	Window::Window(const wchar_t* title, DirectX::XMINT2 size)
	{
		// Fensterklasse registrieren
		WNDCLASS wc = {};
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = GetModuleHandleW(NULL);
		wc.lpszClassName = ClassName;
		m_Instance = wc.hInstance;
		RegisterClassW(&wc);

		// Fenster erstellen
		DWORD style = WS_OVERLAPPEDWINDOW;
		m_Pos = { GetSystemMetrics(SM_CXSCREEN) / 2 - size.x / 2, GetSystemMetrics(SM_CYSCREEN) / 2 - size.y / 2 };
		m_Size = size;
		m_Handle = CreateWindowExW(0, ClassName, title, style, m_Pos.x, m_Pos.y, size.x, size.y, NULL, NULL, m_Instance, NULL);


		// Die Fensterinstanz mit dem Fensterhandle verknüpfen, um in der WindowProc auf die Instanz zugreifen zu können
		SetWindowLongPtrW(static_cast<HWND>(m_Handle), GWLP_USERDATA, (LONG_PTR)this);

		// Fenster anzeigen
		ShowWindow(static_cast<HWND>(m_Handle), SW_NORMAL);
	}

	Window::~Window()
	{
		CloseWindow(static_cast<HWND>(m_Handle));
		DestroyWindow(static_cast<HWND>(m_Handle));
	}

	void Window::pollEvents()
	{
		MSG msg = {};
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

}

