#include "Application.h"
#include <cstdio>

#include "external/ImGui/backends/imgui_impl_win32.h"
#include "external/ImGui/backends/imgui_impl_dx12.h"


#include "external/ImGui/imgui_internal.h"

#include "Events/Callbacks.h"
#include "Logger.h"
#include <thread>

#include "AI/Math/Tensor.h"
#include "AI/Math/TensorMath.h"

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

	std::string formatTensor(AI::Tensor<float>& tensor) {
		std::string result = "Tensor shape: [";
		for (size_t i = 0; i < tensor.getShape().size(); ++i) {
			result += std::to_string(tensor.getShape()[i]);
			if (i < tensor.getShape().size() - 1) result += ", ";
		}
		result += "]\n";
		result += "Values:\n";
		for (size_t i = 0; i < tensor.getTotalSize(); ++i) {
			result += std::to_string(tensor[i]) + " ";
		}
		result += "\n";
		return result;
	}

	void Application::onStart() {
		{
			// Einfaches XOR Model Test
			// 3 Schichten: Input (2 Neuronen), Hidden (8 Neuronen), Output (1 Neuron)
			AI::Tensor<float> inputWeights({ 2, 8 });
			inputWeights = {
				10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f,
				10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f
			};

			AI::Tensor<float> inputBiases({ 8 });
			inputBiases = { 0.0f, 0.0f, 0.0f, 0.0f, -10.0f, -10.0f, -10.0f, -10.0f };

			AI::Tensor<float> hiddenWeights({ 8, 8 });
			hiddenWeights = {
				0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f,
				0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f,
				0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f,
				0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f,
				-0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f,
				-0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f,
				-0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f,
				-0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f, -0.050f
			};


			AI::Tensor<float> hiddenBiases({ 8 });
			hiddenBiases = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

			AI::Tensor<float> outputWeights({ 8, 1 });
			outputWeights = { 0.17098493f, 0.17098493f, 0.17098493f, 0.17098493f, 0.17098493f, 0.17098493f, 0.17098493f, 0.17098493f };

			AI::Tensor<float> outputBiases({ 1 });
			outputBiases = { 0.0f };

			AI::Tensor<float> modelInput({ 1, 2 });
			modelInput = { 1.0f, 1.0f };


			AI::Tensor<float> hiddenLayerInput({ 1, 8 });
			AI::Tensor<float> outputLayerInput({ 1, 8 });
			AI::Tensor<float> modelOutput({ 1, 1 });

			AI::Math::MatMul(modelInput, inputWeights, hiddenLayerInput, AI::Math::CreateBroadcastPlan<float>({ modelInput.view(), inputWeights.view() }));
			Core::Logger::Debug("Hidden Layer Input (after MatMul): {}", formatTensor(hiddenLayerInput));
			auto hiddenView = hiddenLayerInput.view();
			AI::Math::Add<float>(hiddenView, inputBiases.view(), hiddenView, AI::Math::CreateBroadcastPlan<float>({ hiddenView, inputBiases.view() }));
			Core::Logger::Debug("Hidden Layer Input (after Add Biases): {}", formatTensor(hiddenLayerInput));
			AI::Math::SiLU<float>(hiddenLayerInput, hiddenLayerInput);
			Core::Logger::Debug("Hidden Layer Input (after SiLU): {}", formatTensor(hiddenLayerInput));

			AI::Math::MatMul(hiddenLayerInput, hiddenWeights, outputLayerInput, AI::Math::CreateBroadcastPlan<float>({ hiddenLayerInput.view(), hiddenWeights.view() }));
			Core::Logger::Debug("Output Layer Input (after MatMul): {}", formatTensor(outputLayerInput));
			auto outputView = outputLayerInput.view();
			AI::Math::Add<float>(outputView, hiddenBiases.view(), outputView, AI::Math::CreateBroadcastPlan<float>({ outputView, hiddenBiases.view() }));
			Core::Logger::Debug("Output Layer Input (after Add Biases): {}", formatTensor(outputLayerInput));
			AI::Math::SiLU<float>(outputLayerInput, outputLayerInput);
			Core::Logger::Debug("Output Layer Input (after SiLU): {}", formatTensor(outputLayerInput));

			AI::Math::MatMul(outputLayerInput, outputWeights, modelOutput, AI::Math::CreateBroadcastPlan<float>({ outputLayerInput.view(), outputWeights.view() }));
			Core::Logger::Debug("Model Output (after MatMul): {}", formatTensor(modelOutput));
			auto resultsView = modelOutput.view();
			AI::Math::Add<float>(resultsView, outputBiases.view(), resultsView, AI::Math::CreateBroadcastPlan<float>({ resultsView, outputBiases.view() }));
			Core::Logger::Debug("Model Output (after Add Biases): {}", formatTensor(modelOutput));
		}

		{


#include <chrono>

			constexpr size_t M = 1;
			constexpr size_t K = 4096;
			constexpr size_t N = 4096;

			AI::Tensor<float> a({ M, K });
			AI::Tensor<float> b({ K, N });
			AI::Tensor<float> resultTensor({ M, N });

			for (size_t i = 0; i < M; ++i)
			{
				for (size_t k = 0; k < K; ++k)
				{
					a(i, k) =
						static_cast<float>((i + k) % 17) * 0.01f;
				}
			}

			for (size_t k = 0; k < K; ++k)
			{
				for (size_t j = 0; j < N; ++j)
				{
					b(k, j) =
						static_cast<float>((k + j) % 13) * 0.01f;
				}
			}

			auto plan = AI::Math::CreateBroadcastPlan<float>({
				a.view(),
				b.view()
				});

			assert(plan.matMulValid);

			constexpr size_t warmupIterations = 1;
			constexpr size_t iterations = 3;

			for (size_t i = 0; i < warmupIterations; ++i)
			{
				AI::Math::MatMul(
					a,
					b,
					resultTensor,
					plan
				);
			}

			auto start = std::chrono::steady_clock::now();

			for (size_t i = 0; i < iterations; ++i)
			{
				AI::Math::MatMul(
					a,
					b,
					resultTensor,
					plan
				);
			}

			auto end = std::chrono::steady_clock::now();

			const double totalMs =
				std::chrono::duration<double, std::milli>(
					end - start
				).count();

			const double averageMs =
				totalMs / static_cast<double>(iterations);

			const double flops =
				2.0 *
				static_cast<double>(M) *
				static_cast<double>(N) *
				static_cast<double>(K);

			const double gflops =
				flops / (averageMs / 1000.0) / 1e9;

			Core::Logger::Debug(
				"MatMul {}x{} @ {}x{}",
				M, K,
				K, N
			);

			Core::Logger::Debug(
				"Average: {} ms | {} GFLOP/s",
				averageMs,
				gflops
			);

			Core::Logger::Debug(
				"Result samples: {}, {}, {}",
				resultTensor(0, 0),
				resultTensor(M / 2, N / 2),
				resultTensor(M - 1, N - 1)
			);
		}
		
		//Erstellt das Fenster, den Grafik-Kontext, die Swapchain und den Renderer.
		m_EventSystem = new Events::EventSystem();
		m_Window = new Window(L"Application");
		m_Context = new Graphics::GraphicsContext();
		m_Renderer = new Graphics::Renderer();
		m_Swapchain = new Graphics::Swapchain(m_Window);

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(m_Window->getHandle());
		m_Renderer->initImGui();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		Game::GameSettings settings;
		settings.showColliders = false;		settings.levelPath = "../../Content/Levels/testLevel.lvl";

		m_GameEditor = new Game::GameEditor();
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
			if (m_Mode == ApplcationMode::Editor) m_GameEditor->update(TS);
			else if(m_Mode == ApplcationMode::Game) m_GameInstance->update(TS);
			auto timeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			m_Renderer->beginFrame();

			if (m_Mode == ApplcationMode::Editor) m_GameEditor->render();
			else if (m_Mode == ApplcationMode::Game) {
				m_Renderer->beginRenderTarget(m_GameInstance->getTarget(), m_GameInstance->m_CameraPosition, 1.0f / m_GameInstance->m_CameraZoom);

				m_GameInstance->render();

				m_Renderer->drawRects();
				m_Renderer->endRenderTarget(m_GameInstance->getTarget());

			}

			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos({ viewport->WorkPos.x, viewport->WorkPos.y });
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("Main", 0, window_flags);

			ImGui::PopStyleVar(2);

			ImGuiID dockspace_id = ImGui::GetID("Dockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_AutoHideTabBar);

			if (m_Mode == ApplcationMode::Editor) m_GameEditor->drawGUI();
			else if(m_Mode == ApplcationMode::Game) {
				ImGuiIO& io = ImGui::GetIO();


				ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Always);


				ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

				ImDrawList* drawList = ImGui::GetWindowDrawList();

				float gameAspectRatioX = (float)m_GameInstance->getTarget()->getSize().x / (float)m_GameInstance->getTarget()->getSize().y;


				uint32_t height = io.DisplaySize.y;
				uint32_t width = height * gameAspectRatioX;

				uint32_t xOffset = (io.DisplaySize.x - width) / 2;
				uint32_t yOffset = 0;

				if (io.DisplaySize.x < width) {
					width = io.DisplaySize.x;
					height = width / gameAspectRatioX;
					xOffset = 0;
					yOffset = (io.DisplaySize.y - height) / 2;
				}

				drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest, nullptr);
		
				drawList->AddImage((ImTextureID)(uintptr_t)m_Renderer->getTextureManager().getSRVGPUDescriptorHandle(m_GameInstance->getTarget()->getSRVDescriptorIndex()).ptr, ImVec2(xOffset, yOffset), ImVec2(width + xOffset, height + yOffset));
				drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);
				
				ImGui::End();
			}

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("Exit")) {
						m_RequestExit = true;
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("DevTools")) {
					if (ImGui::BeginMenu("Application Mode")) {
						if (ImGui::MenuItem("Editor", nullptr, m_Mode == ApplcationMode::Editor)) {
							m_Mode = ApplcationMode::Editor;
						}
						if (ImGui::MenuItem("Game", nullptr, m_Mode == ApplcationMode::Game)) {
							m_Mode = ApplcationMode::Game;
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			ImGui::End();

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

