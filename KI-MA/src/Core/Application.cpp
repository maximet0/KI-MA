#include "Application.h"
#include <cstdio>

#include "external/ImGui/backends/imgui_impl_win32.h"
#include "external/ImGui/backends/imgui_impl_dx12.h"


#include "external/ImGui/imgui_internal.h"

#include "Events/Callbacks.h"
#include "Logger.h"
#include <thread>
#include <random>
#include <numeric>

#include "AI/Model.h"
#include "AI/Tensor.h"
#include "AI/Math/TensorMath.h"
#include "AI/Math/GPUTensorMath.h"




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

	uint32_t swapEndianess(uint32_t value) {
		return ((value >> 24) & 0x000000FF) |
			((value >> 8) & 0x0000FF00) |
			((value << 8) & 0x00FF0000) |
			((value << 24) & 0xFF000000);
	}

	void Application::onStart() {
		

		//Transpose Performance Test
		{
			
			AI::Tensor<float> testTensor({ 4096, 4096 });
			std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
			for (size_t i = 0; i < 10; ++i) {
				AI::Math::Transpose(testTensor, testTensor);
			}
			Core::Logger::Debug("Transpose Time elapsed: {} seconds", std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count());

		}

		// MNIST Model Test
		/* {
			// Load images and labels from MNIST dataset
			std::ifstream mnistImages("C:\\Users\\maxim\\Downloads\\train-images-idx3-ubyte\\train-images.idx3-ubyte", std::ios::in | std::ios::binary);

			// Big-endian
			struct imgHDR {
				uint32_t magic;
				uint32_t imageCount;
				uint32_t rows;
				uint32_t cols;
			} header;

			mnistImages.read(reinterpret_cast<char*>(&header), sizeof(header));

			header.magic = swapEndianess(header.magic);
			header.imageCount = swapEndianess(header.imageCount);
			header.rows = swapEndianess(header.rows);
			header.cols = swapEndianess(header.cols);

			AI::Tensor<float> mnistImageData({ header.imageCount, header.rows * header.cols });

			char* rawImageData = new char[header.imageCount * header.rows * header.cols];
			mnistImages.read(reinterpret_cast<char*>(rawImageData), header.imageCount* header.rows* header.cols);
			for (size_t i = 0; i < header.imageCount * header.rows * header.cols; ++i) {
				mnistImageData[i] = static_cast<float>(static_cast<unsigned char>(rawImageData[i])) / 255.0f;
			}
			delete[] rawImageData;
			mnistImages.close();

			Core::Logger::Success("MNIST Image Header: Magic: {}, Image Count: {}, Rows: {}, Cols: {}", header.magic, header.imageCount, header.rows, header.cols);

			std::ifstream mnistLabels("C:\\Users\\maxim\\Downloads\\train-labels-idx1-ubyte\\train-labels.idx1-ubyte", std::ios::in | std::ios::binary);

			struct lblHDR {
				uint32_t magic;
				uint32_t labelCount;
			} labelHeader;

			mnistLabels.read(reinterpret_cast<char*>(&labelHeader), sizeof(labelHeader));

			labelHeader.magic = swapEndianess(labelHeader.magic);
			labelHeader.labelCount = swapEndianess(labelHeader.labelCount);

			char* imgLabels = new char[labelHeader.labelCount];
			mnistLabels.read(imgLabels, labelHeader.labelCount);
			mnistLabels.close();

			Core::Logger::Success("MNIST Label Header: Magic: {}, Label Count: {}", labelHeader.magic, labelHeader.labelCount);

			AI::Model mnistModel;
			mnistModel.addLayer(AI::LayerType::Input, AI::ActivationFunction::None, 784);
			mnistModel.addLayer(AI::LayerType::Dense, AI::ActivationFunction::ReLU, 800);
			//mnistModel.addLayer(AI::LayerType::Dense, AI::ActivationFunction::ReLU, 256);
			mnistModel.addLayer(AI::LayerType::Output, AI::ActivationFunction::Softmax, 10);
			mnistModel.buildModel(60);

			mnistModel.initializeParameters();
			AI::Math::AdamOptimizerParams adamParams;
			adamParams = { 0.001f, 0.9f, 0.999f, 1e-8f, 0.01f };

			mnistModel.setOptimizer(adamParams);

			std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
			mnistModel.beginTraining();

			std::mt19937 rng(std::random_device{}());
			std::uniform_int_distribution<int> shiftDistr(-4, 4);

			const size_t rows = header.rows;
			const size_t cols = header.cols;
			const size_t pixels = rows * cols;

			AI::Tensor<float> shiftedImages({ 60, pixels });	
			std::fill_n(shiftedImages.getDataPtr(), 60 * pixels, 0.0f);


			for (size_t epoch = 0; epoch < 5; ++epoch) {
				float combinedLossforEpoch = 0.0f;

				size_t correctCount = 0;
				for (size_t i = 0; i < labelHeader.labelCount; i += 60) {

					AI::Tensor<float> inputImage = mnistImageData.view({ 60, header.rows * header.cols }, i, 0).viewCopy();

					std::fill_n(shiftedImages.getDataPtr(), 60 * pixels, 0.0f);

					for (size_t j = 0; j < 60; ++j) {
						const int dx = shiftDistr(rng);
						const int dy = shiftDistr(rng);

						const float* source = inputImage.getDataPtr() + j * pixels;
						float* destination = shiftedImages.getDataPtr() + j * pixels;

						for (size_t y = 0; y < rows; ++y) {
							const int sourceY = static_cast<int>(y) - dy;

							if (sourceY < 0 || sourceY >= static_cast<int>(rows))
								continue;

							for (size_t x = 0; x < cols; ++x) {
								const int sourceX = static_cast<int>(x) - dx;

								if (sourceX < 0 || sourceX >= static_cast<int>(cols))
									continue;

								destination[y * cols + x] =
									source[static_cast<size_t>(sourceY) * cols
									+ static_cast<size_t>(sourceX)];
							}
						}
					}

					// Copy back into this batch's independent image buffer.
					inputImage.clearCachedData();
					inputImage = shiftedImages;

					AI::Tensor<float> targetOutput({ 60, 10 });
					memset(targetOutput.getDataPtr(), 0, sizeof(float) * 60 * 10);
					for (size_t j = 0; j < 60; ++j) {
						targetOutput(j, imgLabels[i + j]) = 1.0f;
					}
					AI::Tensor<float> modelOutput({ 60, 10 });
					mnistModel.forward(inputImage, modelOutput);
					for (size_t j = 0; j < 60; ++j) {
						size_t predictedLabel = std::distance(modelOutput.getDataPtr() + j * 10, std::max_element(modelOutput.getDataPtr() + j * 10, modelOutput.getDataPtr() + (j + 1) * 10));
						if (predictedLabel == static_cast<size_t>(imgLabels[i + j])) {
							correctCount++;
						}
					}

					mnistModel.trainSupervised(targetOutput);
					combinedLossforEpoch += mnistModel.getCurrentLoss();
				}
				Core::Logger::Debug("Epoch {}: Average Loss: {}", epoch, combinedLossforEpoch / static_cast<float>(labelHeader.labelCount / 60));
				Core::Logger::Debug("Current Accuracy: {}%", (static_cast<float>(correctCount) / static_cast<float>(labelHeader.labelCount)) * 100.0f);
				correctCount = 0;
				Core::Logger::Debug("Time elapsed: {} seconds", std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count());
			}
			mnistModel.endTraining();

			delete[] imgLabels;
			
			//mnistModel.buildModel(1); // Rebuild the model for inference with batch size 1

			// Check Test Images
			std::ifstream mnistTestImages("C:\\Users\\maxim\\Downloads\\t10k-images-idx3-ubyte\\t10k-images.idx3-ubyte", std::ios::in | std::ios::binary);


			mnistTestImages.read(reinterpret_cast<char*>(&header), sizeof(header));

			header.magic = swapEndianess(header.magic);
			header.imageCount = swapEndianess(header.imageCount);
			header.rows = swapEndianess(header.rows);
			header.cols = swapEndianess(header.cols);

			AI::Tensor<float> mnistImageTestData({ header.imageCount, header.rows * header.cols });

			rawImageData = new char[header.imageCount * header.rows * header.cols];
			mnistTestImages.read(reinterpret_cast<char*>(rawImageData), header.imageCount* header.rows* header.cols);
			for (size_t i = 0; i < header.imageCount * header.rows * header.cols; ++i) {
				mnistImageTestData[i] = static_cast<float>(static_cast<unsigned char>(rawImageData[i])) / 255.0f;
			}
			delete[] rawImageData;
			mnistTestImages.close();

			Core::Logger::Success("MNIST Image Header: Magic: {}, Image Count: {}, Rows: {}, Cols: {}", header.magic, header.imageCount, header.rows, header.cols);

			std::ifstream mnistTestLabels("C:\\Users\\maxim\\Downloads\\t10k-labels-idx1-ubyte\\t10k-labels.idx1-ubyte", std::ios::in | std::ios::binary);

			mnistTestLabels.read(reinterpret_cast<char*>(&labelHeader), sizeof(labelHeader));

			labelHeader.magic = swapEndianess(labelHeader.magic);
			labelHeader.labelCount = swapEndianess(labelHeader.labelCount);

			imgLabels = new char[labelHeader.labelCount];
			mnistTestLabels.read(imgLabels, labelHeader.labelCount);
			mnistTestLabels.close();

			float avgLoss = 0.0f;
			size_t correctCount = 0;
			size_t randomCorrectCount = 0;

			for (size_t i = 0; i + 60 <= labelHeader.labelCount; i += 60) {
				AI::Tensor<float> inputImage = mnistImageTestData.view({ 60, header.rows * header.cols }, i, 0).viewCopy();
				AI::Tensor<float> modelOutput({ 60, 10 });
				mnistModel.forward(inputImage, modelOutput);
				avgLoss += mnistModel.getCurrentLoss();
				for (size_t j = 0; j < 60; ++j) {
					size_t predictedLabel = std::distance(modelOutput.getDataPtr() + j * 10, std::max_element(modelOutput.getDataPtr() + j * 10, modelOutput.getDataPtr() + (j + 1) * 10));
					if (predictedLabel == static_cast<size_t>(imgLabels[i + j])) {
						++correctCount;
					}

					if (rand() % 10 == static_cast<size_t>(imgLabels[i + j])) {
						++randomCorrectCount;
					}
				}
			}

			mnistModel.saveModel("models/mnist.modl");


			Core::Logger::Debug("MNIST Test Accuracy: {}%", (static_cast<float>(correctCount) / static_cast<float>(labelHeader.labelCount)) * 100.0f);
			Core::Logger::Debug("MNIST Test Random Accuracy: {}%", (static_cast<float>(randomCorrectCount) / static_cast<float>(labelHeader.labelCount)) * 100.0f);
			Core::Logger::Debug("MNIST Test Average Loss: {}", avgLoss / static_cast<float>(labelHeader.labelCount));
			delete[] imgLabels;
		}*/

		
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

		AI::Math::GPUInit();

		

		AI::Math::beginGPUMath();



		m_ModelEditor = new AI::ModelEditor();

		AI::Math::endGPUMath();


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
			else if(m_Mode == ApplcationMode::ModelEditor) m_ModelEditor->render();
			else if(m_Mode == ApplcationMode::Game) m_GameInstance->update(TS);
			auto timeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			m_Renderer->beginFrame();

			if (m_Mode == ApplcationMode::Editor) m_GameEditor->render();
			else if (m_Mode == ApplcationMode::ModelEditor) m_ModelEditor->update(TS);
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
			else if(m_Mode == ApplcationMode::ModelEditor) m_ModelEditor->drawGUI();
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
						if (ImGui::MenuItem("Model Editor", nullptr, m_Mode == ApplcationMode::ModelEditor)) {
							m_Mode = ApplcationMode::ModelEditor;
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

