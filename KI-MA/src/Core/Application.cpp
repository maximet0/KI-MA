#include "Application.h"
#include <cstdio>

#include "external/ImGui/backends/imgui_impl_win32.h"
#include "external/ImGui/backends/imgui_impl_dx12.h"

#include "Events/Callbacks.h"
#include "Logger.h"
#include <thread>

namespace Core {
	Application* Application::s_Application = nullptr;


	Application* Core::Application::getApplication()
	{
		return s_Application;
	}

	Application::Application() {
		// Speichert die aktualle Instanz der Anwendung falls noch keine existiert.
		if (s_Application == nullptr) s_Application = this;
		else printf("[WARNING] Application instance already exists!\n");
	}

	Application::~Application() {
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}


	DirectX::XMINT2 g_oldSize;

	constexpr float TS = 1.0f / 60.0f;
	constexpr float FS = 1.0f / 60.0f;

	double g_Accumulator = 0.0;
	double g_RenderAccumulator = 0.0;

	void Application::onStart() {
		//Erstellt das Fenster, den Grafik-Kontext, die Swapchain und den Renderer.
		m_EventSystem = new Events::EventSystem();
		m_Window = new Window(L"Application");
		m_Context = new Graphics::GraphicsContext();
		m_Renderer = new Graphics::Renderer();
		m_Swapchain = new Graphics::Swapchain(m_Window);

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(m_Window->getHandle());
		m_Renderer->initImGui();

		Game::GameSettings settings;
		settings.levelEditorMode = true;
		settings.levelPath = "testLevel.lvl";

		m_GameInstance = new Game::GameInstance(settings);
	}

	void Application::onUpdate() {
		if (m_Window->getSize().x != g_oldSize.x || m_Window->getSize().y != g_oldSize.y)
		{
			m_Renderer->waitForGPU();
			m_Swapchain->resize(m_Window->getSize());
		}

		static auto lastTime = std::chrono::steady_clock::now();
		auto currentTime = std::chrono::steady_clock::now().time_since_epoch().count() / 1000000000.0;


		double deltaTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - lastTime).count();
		lastTime = std::chrono::steady_clock::now();

		deltaTime = (std::min)(deltaTime, 0.25);

		g_Accumulator += deltaTime;
		g_RenderAccumulator += deltaTime;

		while (g_Accumulator >= TS) {
			g_Accumulator -= TS;
			m_GameInstance->update(TS);
		}

		while (g_RenderAccumulator >= FS) {
			g_RenderAccumulator -= FS;

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			m_Renderer->beginFrame();
			m_GameInstance->render();

			//ImGui::ShowDemoWindow();

			m_GameInstance->drawGUI();
			m_Renderer->endFrame();
		}


		g_oldSize = m_Window->getSize();
		m_Window->pollEvents();
		m_EventSystem->pollEvents();

		double sleepTime = (std::min)(TS - g_Accumulator, FS - g_RenderAccumulator);
		if (sleepTime > 0.0) {
			std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
		}
	}

}

