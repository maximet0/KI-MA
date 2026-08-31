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
		else Logger::Fatal("Application instance already exists!\n");
	}

	Application::~Application() {
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}


	DirectX::XMINT2 g_oldSize;




	void preciseSleep(double seconds) {
		auto end = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
		while (std::chrono::steady_clock::now() < end) {
			std::this_thread::yield();
		}
	}

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
		settings.levelPath = "../../testLevel.lvl";

		m_GameInstance = new Game::GameInstance(settings);

		if (!std::filesystem::exists("TextureSets")) {
			std::filesystem::create_directory("TextureSets");
		}

		m_Renderer->getTextureManager().beginEarlyTextureLoad();
		for (std::filesystem::directory_entry entry : std::filesystem::directory_iterator("TextureSets")) {
			if (entry.is_regular_file() && entry.path().extension() == ".txst") {
				m_Renderer->getTextureManager().loadTextureSet(entry.path());
			}
		}
		m_Renderer->getTextureManager().endEarlyTextureLoad();
	}

	constexpr float TS = 1.0f / 60.0f;

	float TimeScale = 1.0f;

	void Application::onUpdate() {
		if (m_Window->getSize().x != g_oldSize.x || m_Window->getSize().y != g_oldSize.y)
		{
			m_Renderer->waitForGPU();
			m_Swapchain->resize(m_Window->getSize());
		}

		static auto lastTime = std::chrono::steady_clock::now();
		static auto nextTickTime = std::chrono::steady_clock::now() + std::chrono::duration<double>(TS / TimeScale);
		auto currentTime = std::chrono::steady_clock::now();

		double deltaTime = std::chrono::duration<double>(currentTime - lastTime).count();
		lastTime = currentTime;

		if (currentTime >= nextTickTime) {
			nextTickTime += std::chrono::duration<double>(TS / TimeScale);

			if(nextTickTime < currentTime)
				nextTickTime = currentTime + std::chrono::duration<double>(TS / TimeScale);

			auto start = std::chrono::high_resolution_clock::now();
			m_GameInstance->update(TS);
			auto timeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			m_Renderer->beginFrame();
			m_GameInstance->render();

			m_GameInstance->drawGUI();
			m_Renderer->endFrame();
		}


		g_oldSize = m_Window->getSize();
		m_Window->pollEvents();
		m_EventSystem->pollEvents();
		
		double sleepTime = std::chrono::duration<double>(nextTickTime - currentTime).count();
		if (sleepTime > 0.0) {
			preciseSleep(sleepTime);
		}
	}

}

