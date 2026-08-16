#include "Window.h"

#include "external/ImGui/backends/imgui_impl_win32.h"
#include "Application.h"

#include <Windows.h>
#include <windowsx.h>


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


namespace Core {
	const wchar_t ClassName[] = L"WINDOW";

	LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		Application* app = Application::getApplication();
		
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
		case WM_CLOSE: {
			app->getEventSystem()->registerEvent(new Events::WindowCloseEvent(window));
			//DestroyWindow(hwnd);
			return 0;
		}
		case WM_ERASEBKGND:
			return 1; // Hintergrund nicht löschen, um Flackern zu vermeiden
		case WM_MOUSEMOVE: {
			POINT absolutePos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ClientToScreen(hwnd, &absolutePos);
			app->getEventSystem()->registerEvent(new Events::MousePosEvent(window, { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, { absolutePos.x, absolutePos.y }));
			break;
		}
		case WM_NCMOUSEMOVE: {
			POINT mousePos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(hwnd, &mousePos);
			app->getEventSystem()->registerEvent(new Events::MousePosEvent(window, { mousePos.x, mousePos.y }, { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }));
			break;
		}
		case WM_MOUSELEAVE:
		case WM_NCMOUSELEAVE:
		{
			app->getEventSystem()->registerEvent(new Events::MouseLeaveEvent(window));
			break;
		}
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
		{
			Events::MouseButton button;
			if (uMsg == WM_LBUTTONDBLCLK) { button = Events::MouseButton::Left; }
			if (uMsg == WM_RBUTTONDBLCLK) { button = Events::MouseButton::Right; }
			if (uMsg == WM_MBUTTONDBLCLK) { button = Events::MouseButton::Middle; }
			if (uMsg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? Events::MouseButton::Extra1 : Events::MouseButton::Extra2; }
			SetCapture(hwnd);
			app->getEventSystem()->registerEvent(new Events::MouseButtonEvent(window, Events::KeyState::Clicked, button));
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
		{
			Events::MouseButton button;
			if (uMsg == WM_LBUTTONDOWN) { button = Events::MouseButton::Left; }
			if (uMsg == WM_RBUTTONDOWN) { button = Events::MouseButton::Right; }
			if (uMsg == WM_MBUTTONDOWN) { button = Events::MouseButton::Middle; }
			if (uMsg == WM_XBUTTONDOWN) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? Events::MouseButton::Extra1 : Events::MouseButton::Extra2; }
			SetCapture(hwnd);
			app->getEventSystem()->registerEvent(new Events::MouseButtonEvent(window, Events::KeyState::Down, button));
			return 0;
		}

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		{
			Events::MouseButton button;
			if (uMsg == WM_LBUTTONUP) { button = Events::MouseButton::Left; }
			if (uMsg == WM_RBUTTONUP) { button = Events::MouseButton::Right; }
			if (uMsg == WM_MBUTTONUP) { button = Events::MouseButton::Middle; }
			if (uMsg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? Events::MouseButton::Extra1 : Events::MouseButton::Extra2; }
			ReleaseCapture();
			app->getEventSystem()->registerEvent(new Events::MouseButtonEvent(window, Events::KeyState::Up, button));
			return 0;
		}
		case WM_MOUSEWHEEL:
			app->getEventSystem()->registerEvent(new Events::MouseWheelEvent(window, DirectX::XMINT2(0, static_cast<int>(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA))));
			return 0;
		case WM_MOUSEHWHEEL:
			app->getEventSystem()->registerEvent(new Events::MouseWheelEvent(window, DirectX::XMINT2(static_cast<int>(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA), 0)));
			return 0;
		case WM_CHAR:
			app->getEventSystem()->registerEvent(new Events::KeyboardCharEvent(window, static_cast<wchar_t>(wParam)));
			return 0;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			Events::KeyboardKey key = static_cast<Events::KeyboardKey>(wParam);
			int scancode = static_cast<int>(LOBYTE(HIWORD(lParam)));

			if (key == VK_SHIFT) {
				if (scancode == MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC)) key = Events::KeyboardKey::Key_LSHIFT;
				else if(scancode == MapVirtualKey(VK_RSHIFT, MAPVK_VK_TO_VSC)) key = Events::KeyboardKey::Key_RSHIFT;
			}

			BOOL extended = (lParam >> 24) & 1; // R / L
			
			if (key == VK_CONTROL) {
				if(extended)  key = Events::KeyboardKey::Key_RCONTROL;
				else  key = Events::KeyboardKey::Key_LCONTROL;
			}
			else if (key == VK_MENU) {
				if (extended)  key = Events::KeyboardKey::Key_RALT;
				else  key = Events::KeyboardKey::Key_LALT;
			}

			app->getEventSystem()->registerEvent(new Events::KeyboardKeyEvent(window, Events::KeyState::Down, key, scancode));
			return 0;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			Events::KeyboardKey key = static_cast<Events::KeyboardKey>(wParam);
			int scancode = static_cast<int>(LOBYTE(HIWORD(lParam)));

			if (key == VK_SHIFT) {
				if (scancode == MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC)) key = Events::KeyboardKey::Key_LSHIFT;
				else if (scancode == MapVirtualKey(VK_RSHIFT, MAPVK_VK_TO_VSC)) key = Events::KeyboardKey::Key_RSHIFT;
			}

			BOOL extended = (lParam >> 24) & 1; // R / L

			if (key == VK_CONTROL) {
				if (extended)  key = Events::KeyboardKey::Key_RCONTROL;
				else  key = Events::KeyboardKey::Key_LCONTROL;
			}
			else if (key == VK_MENU) {
				if (extended)  key = Events::KeyboardKey::Key_RALT;
				else  key = Events::KeyboardKey::Key_LALT;
			}


			app->getEventSystem()->registerEvent(new Events::KeyboardKeyEvent(window, Events::KeyState::Up, key, scancode));
			return 0;
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

