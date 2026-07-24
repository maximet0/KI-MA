#include "Application.h"
#include <cstdio>

#include "external/ImGui/backends/imgui_impl_win32.h"
#include "external/ImGui/backends/imgui_impl_dx12.h"

#include "Events/Callbacks.h"
#include "Logger.h"

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

	Graphics::RenderTarget* renderTarget[200];

	float spriteX[200];
	float spriteY[200];

	int32_t focusedInstance = -1;

	bool keys[256] = { 0 };

	void keyboardCallback(Core::Window* window, Events::KeyState state, Events::KeyboardKey key, int scancode) {
		if (state == Events::KeyState::Down || state == Events::KeyState::Clicked) keys[(uint8_t)key] = true;
		else if (state == Events::KeyState::Up) keys[(uint8_t)key] = false;
	}

	void Application::onStart() {
		//Erstellt das Fenster, den Grafik-Kontext, die Swapchain und den Renderer.
		m_EventSystem = new Events::EventSystem();
		m_Window = new Window(L"Application");
		m_Context = new Graphics::GraphicsContext();
		m_Renderer = new Graphics::Renderer();
		m_Swapchain = new Graphics::Swapchain(m_Window);
		for (uint32_t i = 0; i < 200; i++) {
			renderTarget[i] = new Graphics::RenderTarget({ 1280, 720 });
		}
		
		m_EventSystem->registerCallback<Events::KeyboardKeyCallback>(Events::EventType::KEYBOARD_KEY, keyboardCallback);

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

		if (focusedInstance != -1) {
			if (keys[Events::KeyboardKey::Key_Right]) {
				spriteX[focusedInstance] += 1.0f;
			}

			if (keys[Events::KeyboardKey::Key_Left]) {
				spriteX[focusedInstance] -= 1.0f;
			}

			if (keys[Events::KeyboardKey::Key_Up]) {
				spriteY[focusedInstance] -= 1.0f;
			}

			if (keys[Events::KeyboardKey::Key_Down]) {
				spriteY[focusedInstance] += 1.0f;
			}
		}
		
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		static bool open = true;
		ImGui::ShowDemoWindow(&open);

		ImGui::Begin("Render Targets");
		for (uint32_t i = 0; i < 200; i++)
		{
			auto textureHandle = m_Renderer->getSRVGPUDescriptorHandle(renderTarget[i]->getSRVDescriptorIndex());
			ImGui::Text("Render Target %d %d", i, textureHandle.ptr);
			ImGui::Image((ImTextureID)(uintptr_t)textureHandle.ptr, ImVec2(renderTarget[i]->getSize().x / 2, renderTarget[i]->getSize().y / 2));

			if (ImGui::IsItemHovered()) {
				focusedInstance = i;
			}

		}
		
		ImGui::End();

		Logger::Debug("Focused Instance: {}", focusedInstance);


		m_Renderer->beginFrame();
		
		for (uint32_t i = 0; i < 200; i++)
		{
			m_Renderer->beginRenderTarget(renderTarget[i]);
			for (uint32_t x = 0; x < 10; x++)
			{
				for (uint32_t y = 0; y < 10; y++) {
					m_Renderer->submitRect({ 64.0f + x * 64.0f, 64.0f + y * 64.0f }, { 32.0f, 32.0f }, 0);
				}

			}

			m_Renderer->submitRect({ 64.0f + spriteX[i], 64.0f + spriteY[i] }, { 64.0f, 64.0f }, 0);
			m_Renderer->drawRects();


			m_Renderer->endRenderTarget(renderTarget[i]);
		}


		
		m_Renderer->endFrame();
		g_oldSize = m_Window->getSize();
		m_Window->pollEvents();
		m_EventSystem->pollEvents();
	}

}

