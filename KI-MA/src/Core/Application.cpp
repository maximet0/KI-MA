#include "Application.h"
#include <cstdio>

#include "ImGui/backends/imgui_impl_win32.h"
#include "ImGui/backends/imgui_impl_dx12.h"

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

	void Application::onStart() {
		//Erstellt das Fenster, den Grafik-Kontext, die Swapchain und den Renderer.
		m_Window = new Window(L"Application");
		m_Context = new Graphics::GraphicsContext();
		m_Swapchain = new Graphics::Swapchain(m_Window);
		m_Renderer = new Graphics::Renderer();
	
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(m_Window->getHandle());
		m_Renderer->initImGui();
	}

	DirectX::XMINT2 g_oldSize;

	void Application::onUpdate() {
		if (m_Window->getSize().x != g_oldSize.x || m_Window->getSize().y != g_oldSize.y)
		{
			m_Renderer->waitForGPU();
			m_Swapchain->resize(m_Window->getSize());
		}

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		static bool open = true;
		ImGui::ShowDemoWindow(&open);

		m_Renderer->beginFrame();
		m_Renderer->drawRectangle();
		ImGui::Render();
		m_Renderer->renderImGui();
		m_Renderer->endFrame();
		g_oldSize = m_Window->getSize();
		m_Window->pollEvents();
	}

}

