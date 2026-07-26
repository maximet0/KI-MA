#pragma once
#include "Window.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/Swapchain.h"
#include "Events/EventSystem.h"

#include "Game/GameInstance.h"

namespace Core {

	// Hauptkalsse der Anwendung
	class Application {
	public:
		/// <summary>
		/// Diese Funktion gibt die aktuelle Instanz der Anwendung zurück.
		/// </summary>
		/// <returns></returns>
		static Application* getApplication();

		Application();
		~Application();

		/// <summary>
		/// Wird beim Start der Anwendung aufgerufen.
		/// </summary>
		void onStart();

		/// <summary>
		/// Wird jedes Frame aufgerufen, um die Anwendung zu aktualisieren.
		/// </summary>
		void onUpdate();

		/// <summary>
		/// Gibt zurück, ob die Anwendung noch läuft.
		/// </summary>
		/// <returns></returns>
		bool isRunning() { return m_IsRunning; };

		/// <summary>
		/// Gibt den Grafik-Kontext der Anwendung zurück.
		/// </summary>
		/// <returns></returns>
		Graphics::GraphicsContext* getGraphicsContext() { return m_Context; };

		/// <summary>
		/// Gibt die Swapchain der Anwendung zurück.
		/// </summary>
		/// <returns></returns>
		Graphics::Swapchain* getSwapchain() { return m_Swapchain; };

		/// <summary>
		/// Gibt den Renderer der Anwendung zurück.
		/// </summary>
		/// <returns></returns>
		Graphics::Renderer* getRenderer() { return m_Renderer; };

		/// <summary>
		/// Gibt das Event-System der Anwendung zurück.
		/// </summary>
		/// <returns></returns>
		Events::EventSystem* getEventSystem() { return m_EventSystem; };

		/// <summary>
		/// Gibt das Fenster der Anwendung zurück.
		/// </summary>
		/// <returns></returns>
		Window* getWindow() { return m_Window; };

		/// <summary>
		/// Schließt die Anwendung.
		/// </summary>
		void exit() { m_IsRunning = false; };

	private:
		bool m_IsRunning = true;
		Window* m_Window;
		Graphics::GraphicsContext* m_Context;

		Graphics::Swapchain* m_Swapchain;
		Graphics::Renderer* m_Renderer;

		Events::EventSystem* m_EventSystem;

		static Application* s_Application;

		Game::GameInstance* m_GameInstance;

	};
}

