#pragma once
#include <DirectXMath.h>
#include <Windows.h>

namespace Core {

	/// <summary>
	/// Klasse für die Erstellung und Verwaltung eines Win32 Fensters.
	/// </summary>
	class Window {
	public:
		
		Window(const wchar_t* title, DirectX::XMINT2 size = { 1280, 720 });
		~Window();

		/// <summary>
		/// Alle anstehenden Fensterereignisse abfragen und verarbeiten.
		/// </summary>
		void pollEvents();

		/// <summary>
		/// Gibt die Größe des Fensters zurück.
		/// </summary>
		/// <returns></returns>
		DirectX::XMINT2& getSize() { return m_Size; };

		/// <summary>
		/// Gibt die Position des Fensters zurück.
		/// </summary>
		/// <returns></returns>
		DirectX::XMINT2& getPos() { return m_Pos; };

		/// <summary>
		/// Gibt die Native Win32 Fensterhandle zurück.
		/// </summary>
		/// <returns></returns>
		HWND getHandle() { return m_Handle; };

	private:
		static LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		DirectX::XMINT2 m_Size, m_Pos;
		HWND m_Handle;
		HINSTANCE m_Instance;
	};
}

