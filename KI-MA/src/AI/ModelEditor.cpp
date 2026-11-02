#include "ModelEditor.h"

#include <array>
#include <cmath>

#include <fstream>

#include <numbers>
#include <chrono>
#include "external/ImGui/misc/cpp/imgui_stdlib.h"
#include "Math/GPUTensorMath.h"

#include "Core/Application.h"

namespace AI {
	AI::Tensor<float> inputImage({ 20, 28 * 28 });

	DataSet g_TestSet;

	extern std::string formatTensor(AI::Tensor<float>& tensor);

	ModelEditor::ModelEditor() {
		//m_CurrentModel.loadModel("models/mnist.modl", 20);

		// Test CNN
		m_CurrentModel.destroyModel();
		m_CurrentModel.addLayer(LayerType::Input2D, ActivationFunction::None, 4, 4, 2);
		m_CurrentModel.addLayer(LayerType::Conv2D, ActivationFunction::None, 2, 2, 1, 1, 0, 0, 2);
		m_CurrentModel.addLayer(LayerType::Conv2D, ActivationFunction::None, 2, 2, 2, 2, 1, 1, 1);
		m_CurrentModel.addLayer(LayerType::Flatten, ActivationFunction::None);
		m_CurrentModel.addLayer(LayerType::Output, ActivationFunction::None, 2);

		m_CurrentModel.buildModel(1);
		
		Tensor<float> input({ 1, 4, 4, 2 });

		for (size_t i = 0; i < 32; i++)
			input[i] = float(i + 1);

		Tensor<float> conv1Weights({ 8, 2 });
		conv1Weights = {
			 1,  0,
			 0,  1,
			 1, -1,
			 0,  1,
			 0,  1,
			 1,  0,
			-1,  1,
			 1,  0
		};

		Tensor<float> conv1Biases({ 1, 2 });
		conv1Biases = { 1, -2 };

		m_CurrentModel.setLayer(1, conv1Weights, conv1Biases);

		Tensor<float> conv2Weights({ 8, 1 });
		conv2Weights = {
			 1,
			-1,
			 0,
			 1,
			 1,
			 0,
			-1,
			 1
		};

		Tensor<float> conv2Biases({ 1, 1 });
		conv2Biases = { 3 };

		m_CurrentModel.setLayer(2, conv2Weights, conv2Biases);


		Tensor<float> denseWeights({ 4, 2 });
		denseWeights = {
			 1,  0,
			 0,  1,
			-1,  1,
			 2, -1
		};

		Tensor<float> denseBiases({ 1, 2 });
		denseBiases = { 2, -3 };


		m_CurrentModel.setLayer(4, denseWeights, denseBiases);

		Tensor<float> output({ 1, 2 });
		m_CurrentModel.forward(input, output);

		m_CurrentModel.destroyModel();
		m_CurrentModel.loadModel("models/mnist.modl", 20);
		Core::Logger::Debug("Output Tensor: {}", formatTensor(output));

		Math::AdamOptimizerParams adamParams;

		adamParams.learningRate = 0.0005f;
		adamParams.beta1 = 0.9f;
		adamParams.beta2 = 0.999f;
		adamParams.epsilon = 1e-8f;
		adamParams.weightDecay = 0.0005f;

		m_CurrentModel.setOptimizer(adamParams);

		m_BatchSize = 20;

		m_LayerProps.resize(m_CurrentModel.getLayerCount());

		m_CurrentDataSet.loadDataSet("DataSets/mnist.dtst");

		g_TestSet.loadDataSet("C:\\Users\\maxim\\Downloads\\t10k-images-idx3-ubyte\\t10k-images.idx3-ubyte", "C:\\Users\\maxim\\Downloads\\t10k-labels-idx1-ubyte\\t10k-labels.idx1-ubyte");

		srand(time(nullptr));
		size_t randIndex = rand() % m_CurrentDataSet.getDataCount();
		m_CurrentDataSet.getInput(randIndex, 20).copyTo(inputImage);

		AI::Tensor<float> outputImage({ 20, m_CurrentDataSet.getResultSize() });

		m_CurrentModel.beginTraining();
		m_CurrentModel.forwardGPU(inputImage, outputImage);
	}

	float accuracy = 0.0f;

	void ModelEditor::update(float deltaTime) {
	
		if (m_Training && !m_TrainingPaused) {
			AI::Tensor<float> output({ (size_t)m_BatchSize, m_CurrentDataSet.getResultSize() });

			if (m_EpochStep * m_BatchSize >= m_CurrentDataSet.getDataCount()) {
				m_EpochStep = 0;
				m_CurrentEpoch++;

				// Check against test dataset

				uint32_t correctCount = 0;
				uint32_t count = g_TestSet.getDataCount();
				size_t classCount = g_TestSet.getResultSize();

				for (uint32_t i = 0; i + m_BatchSize <= count; i += m_BatchSize) {
					AI::Tensor<float> input = g_TestSet.getInput(i, m_BatchSize);
					AI::Tensor<float> target = g_TestSet.getResult(i, m_BatchSize);
					AI::Tensor<float> output({ (size_t)m_BatchSize, g_TestSet.getResultSize() });
					m_CurrentModel.forwardGPU(input, output);
					for (size_t j = 0; j < m_BatchSize; j++) {
						size_t predictedLabel = 0, actualLabel = 0;

						for (size_t k = 1; k < classCount; k++) {
							if (output[j * classCount + k] > output[j * classCount + predictedLabel]) predictedLabel = k;
							if (target[j * classCount + k] > target[j * classCount + actualLabel]) actualLabel = k;
						}
						if (predictedLabel == actualLabel) {
							correctCount++;
						}
					}
				}
				accuracy = static_cast<float>(correctCount) / static_cast<float>(count);

			}
			else {
				// keep relativly stable fps but don't limit training throughput
				size_t steps = std::min(std::floor(deltaTime / m_StepTime), 1.0);
				m_StepTime = 0.0f;

				for (size_t i = 0; i < steps; i++) {
					std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
					Tensor<float> input = m_CurrentDataSet.getInput(m_EpochStep * m_BatchSize, m_BatchSize);
					m_CurrentModel.forwardGPU(input, output);
					Tensor<float> target = m_CurrentDataSet.getResult(m_EpochStep * m_BatchSize, m_BatchSize);
					m_CurrentModel.trainSupervisedGPU(target);
					m_StepTime = std::max(m_StepTime, std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
				}

				m_EpochStep += steps;
			}

			if (m_CurrentEpoch >= m_TrainingEpochs) {
				m_Training = false;
			}
		}

	}



	void ModelEditor::drawGUI() {
		ImGui::Begin("MNIST TEST");

		static uint32_t selectedBatchIndex = 0;

		if (ImGui::Button("Clear")) {
			std::fill_n(inputImage.getDataPtr() + selectedBatchIndex * 28 * 28, 28 * 28, 0.0f);
			inputImage.markCPUWritten();
		}

		ImGui::SameLine();
		ImGui::DragInt("Batch", reinterpret_cast<int*>(&selectedBatchIndex), 1, 0, m_BatchSize - 1);

		ImGui::InvisibleButton("canvas", ImVec2(280, 280), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		
		if (inputImage.isCPUDataDirty()) inputImage.updateCPUData();

		for (size_t x = 0; x < 28; x++) {
			for (size_t y = 0; y < 28; y++) {
				float pixelValue = inputImage[selectedBatchIndex * 28 * 28 + y * 28 + x];
				ImU32 color = ImColor(pixelValue, pixelValue, pixelValue, 1.0f);
				ImVec2 pos = ImGui::GetItemRectMin();
				pos.x += x * 10;
				pos.y += y * 10;
				ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + 10, pos.y + 10), color);
			}
		}

		static bool wasDragging = false;

		if (ImGui::IsItemHovered() && (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right))) {
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 canvasPos = ImGui::GetItemRectMin();
			int x = (mousePos.x - canvasPos.x) / 10;
			int y = (mousePos.y - canvasPos.y) / 10;
			if (x >= 0 && x < 28 && y >= 0 && y < 28) {
				if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					inputImage[selectedBatchIndex * 28 * 28 + y * 28 + x] = 1.0f;
					inputImage[selectedBatchIndex * 28 * 28 + y * 28 + std::max(0, x - 1)] = std::clamp(inputImage[selectedBatchIndex * 28 * 28 + y * 28 + std::max(0, x - 1)] + 0.2f, 0.0f, 1.0f);
					inputImage[selectedBatchIndex * 28 * 28 + y * 28 + std::min(27, x + 1)] = std::clamp(inputImage[selectedBatchIndex * 28 * 28 + y * 28 + std::min(27, x + 1)] + 0.2f, 0.0f, 1.0f);
					inputImage[selectedBatchIndex * 28 * 28 + std::max(0, y - 1) * 28 + x] = std::clamp(inputImage[selectedBatchIndex * 28 * 28 + std::max(0, y - 1) * 28 + x] + 0.2f, 0.0f, 1.0f);
					inputImage[selectedBatchIndex * 28 * 28 + std::min(27, y + 1) * 28 + x] = std::clamp(inputImage[selectedBatchIndex * 28 * 28 + std::min(27, y + 1) * 28 + x] + 0.2f, 0.0f, 1.0f);
					inputImage.markCPUWritten();
					wasDragging = true;
				}
				else if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
					inputImage[selectedBatchIndex * 28 * 28 + y * 28 + x] = 0.0f;
					inputImage[selectedBatchIndex * 28 * 28 + y * 28 + std::max(0, x - 1)] = std::clamp(inputImage[selectedBatchIndex * 28 * 28 + y * 28 + std::max(0, x - 1)] - 0.2f, 0.0f, 1.0f);
					inputImage[selectedBatchIndex * 28 * 28 + y * 28 + std::min(27, x + 1)] = std::clamp(inputImage[selectedBatchIndex * 28 * 28 + y * 28 + std::min(27, x + 1)] - 0.2f, 0.0f, 1.0f);
					inputImage[selectedBatchIndex * 28 * 28 + std::max(0, y - 1) * 28 + x] = std::clamp(inputImage[selectedBatchIndex * 28 * 28 + std::max(0, y - 1) * 28 + x] - 0.2f, 0.0f, 1.0f);
					inputImage[selectedBatchIndex * 28 * 28 + std::min(27, y + 1) * 28 + x] = std::clamp(inputImage[selectedBatchIndex * 28 * 28 + std::min(27, y + 1) * 28 + x] - 0.2f, 0.0f, 1.0f);
					inputImage.markCPUWritten();
					wasDragging = true;
				}
			}
		}
		

		AI::Tensor<float> outputImage({ 1, 10 });

		if (wasDragging) {
			m_CurrentModel.forwardGPU(inputImage, outputImage);
			wasDragging = false;
		}

		ImGui::End();		

		drawModelVisualization();

		drawTrainingPanel();

		drawModelBrowser();
	}

	void ModelEditor::render() {

	}

	void ModelEditor::drawModelBrowser() {
		ImGui::Begin("Model Browser");

		std::filesystem::path modelDirectory = "models";

		if (!std::filesystem::exists(modelDirectory)) {
			std::filesystem::create_directories(modelDirectory);
		}

		for (const auto& entry : std::filesystem::directory_iterator(modelDirectory)) {
			if (entry.is_regular_file()) {
				const auto& path = entry.path();
				ImGui::PushID(path.string().c_str());
				ImGui::Text("%s", path.filename().string().c_str());
				ImGui::SameLine();
				if(ImGui::Button("Load")) {
					m_CurrentModel.loadModel(path, 20);
					m_LayerProps.clear();
					m_LayerProps.resize(m_CurrentModel.getLayerCount());
					m_CurrentModel.beginTraining();
				}
				ImGui::PopID();
			}
		}

		ImGui::End();
	}

	template<size_t N, typename Func>
	constexpr std::array<ImVec2, N> GenCurve(Func fn) {
		std::array<ImVec2, N> points{};
		for (size_t i = 0; i < N; i++)
		{
			float t = static_cast<float>(i) / (N - 1);
			float x = -1.0f + t * 2.0f;
			float y = fn(x);

			points[i] = { x , std::clamp(y, -1.0f, 1.0f)
			};
		}
		return points;
	}

	inline constexpr auto g_ReLUCurve = GenCurve<24>([](float x) constexpr {
			return x > 0.0f ? x : 0.0f;
		});

	inline const auto g_SiLUCurve = GenCurve<24>([](float x) {
			return x / (1.0f + std::exp(-x));
		});

	template<size_t N>
	void drawCurve(ImVec2 center, float radius, float zoom, const std::array<ImVec2, N>& points, ImU32 color) {

		std::array<ImVec2, N> transformedPoints{};
		const float scale = radius * 0.5f;

		for (size_t i = 0; i < N; i++) {
			transformedPoints[i] = ImVec2(center.x + points[i].x * scale, center.y - points[i].y * scale);
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		
		

		ImColor gridColor = ImColor(color);
		gridColor.Value.w = 0.6f;

		drawList->AddLine(ImVec2(center.x - scale, center.y), ImVec2(center.x + scale, center.y), gridColor, 0.5f * zoom);
		drawList->AddLine(ImVec2(center.x, center.y - scale), ImVec2(center.x, center.y + scale), gridColor, 0.5f * zoom);

		drawList->AddPolyline(transformedPoints.data(), N, color, 0, 1.0f * zoom);

	}


	constexpr size_t maxVerticalNeuronCount = 32; // Maximum number of neurons to display vertically

	constexpr float neuronSize = 10.0f; // Size of each neuron
	constexpr float verticalNeuronSpacing = neuronSize * 3; // Spacing between neurons vertically
	constexpr float horizontalNeuronSpacing = neuronSize * 3; // Spacing between neurons horizontally
	constexpr float layerSpacing = neuronSize * 12; // Spacing between layers


	void ModelEditor::drawLayerVisualization(float xPos, uint32_t layerIndex, float& layerWidth) {
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImU32 neuronColor = ImColor(255, 255, 255, 255);
		const ImU32 outputNeuronColor = ImColor(255, 0, 0, 255);
		const ImU32 inputNeuronColor = ImColor(0, 255, 0, 255);
		const ImU32 canvasBgColor = ImGui::GetColorU32(ImGuiCol_MenuBarBg);
		// Draw Dots on the canvas for each neuron in the layer

		const float textSize = style.FontSizeBase;
		const float textSizeScaled = textSize * m_CanvasCamera.zoom;

		const float headerHeight = textSize + style.FramePadding.x * 2;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const LayerDesc& layerDesc = m_CurrentModel.getLayerDescription(layerIndex);

		size_t connectionCount = 0;

		if (layerDesc.type != LayerType::Output) {
			connectionCount = m_CurrentModel.getLayerDescription(layerIndex + 1).neuronCount;
		}


		ImU32 col = neuronColor;

		if (layerDesc.type == LayerType::Input) col = inputNeuronColor;
		else if (layerDesc.type == LayerType::Dense) col = neuronColor;
		else if (layerDesc.type == LayerType::Output) col = outputNeuronColor;
	
		size_t columns = 1;
		size_t rows = layerDesc.neuronCount;

		if (layerDesc.neuronCount > maxVerticalNeuronCount)
		{
			columns = std::ceil(sqrt((float)layerDesc.neuronCount));
			rows = std::ceil(layerDesc.neuronCount / (float)columns);
		}

		size_t visibleColumns = columns;

		float yBegin = -((rows * verticalNeuronSpacing) / 2.0f);

		if (m_LayerProps[layerIndex].collapsed)
			visibleColumns = 1;

		ImVec2 rectMin = m_CanvasCamera.WorldToScreen(ImVec2(xPos, yBegin));
		ImVec2 rectMax = m_CanvasCamera.WorldToScreen(ImVec2(xPos + visibleColumns * horizontalNeuronSpacing, yBegin + headerHeight + rows * (verticalNeuronSpacing)));
	
		drawList->AddRect(rectMin, rectMax, col, 5.0f, 2.0f * m_CanvasCamera.zoom);

		const char* layerType = "";
		switch (layerDesc.type) {
			case LayerType::Input:
				layerType = "Input";
				break;
			case LayerType::Dense:
				layerType = "Dense";
				break;
			case LayerType::Output:
				layerType = "Output";
				break;
		};

		ImVec2 headerCursorPos = ImVec2(xPos + style.FramePadding.x, yBegin + style.FramePadding.y);



		if (columns > 1) {
			float triangleSize = textSize - 4.0f;
			ImVec2 triA = m_CanvasCamera.WorldToScreen(ImVec2(headerCursorPos.x + 2.0f + (triangleSize) * 0.5f, headerCursorPos.y + 2.0f + (triangleSize)));
			ImVec2 triB = m_CanvasCamera.WorldToScreen(ImVec2(headerCursorPos.x + 2.0f, headerCursorPos.y + 2.0f));
			ImVec2 triC = m_CanvasCamera.WorldToScreen(ImVec2(headerCursorPos.x + 2.0f + (triangleSize), headerCursorPos.y + 2.0f));

			if (m_LayerProps[layerIndex].collapsed) {
				triA = m_CanvasCamera.WorldToScreen(ImVec2(headerCursorPos.x + 2.0f + (triangleSize), headerCursorPos.y + 2.0f + (triangleSize) * 0.5f));
				triB = m_CanvasCamera.WorldToScreen(ImVec2(headerCursorPos.x + 2.0f, headerCursorPos.y + 2.0f));
				triC = m_CanvasCamera.WorldToScreen(ImVec2(headerCursorPos.x + 2.0f, headerCursorPos.y + 2.0f + (triangleSize)));
			}


			drawList->AddTriangleFilled(triA, triB, triC, col);

			if (ImGui::IsItemActive() && ImGui::IsMouseHoveringRect(m_CanvasCamera.WorldToScreen(ImVec2(headerCursorPos.x, headerCursorPos.y)), m_CanvasCamera.WorldToScreen(ImVec2(headerCursorPos.x + textSize, headerCursorPos.y + textSize)))) {
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					m_LayerProps[layerIndex].collapsed = !m_LayerProps[layerIndex].collapsed;
				}
			}

			headerCursorPos.x += textSize + style.ItemInnerSpacing.x;
		}

		if (m_LayerProps[layerIndex].collapsed || columns == 1)
			drawList->AddText(ImGui::GetFont(), textSize * m_CanvasCamera.zoom, m_CanvasCamera.WorldToScreen(ImVec2(xPos + visibleColumns * horizontalNeuronSpacing + style.ItemInnerSpacing.x, yBegin + style.FramePadding.y)), col, std::format("[{}. {}]", layerDesc.neuronCount, connectionCount).c_str());
		else if (m_LayerProps[layerIndex].collapsed == false && columns > 7)
			drawList->AddText(ImGui::GetFont(), textSize * m_CanvasCamera.zoom, m_CanvasCamera.WorldToScreen(headerCursorPos), col, std::format("{} Layer {}, [{}, {}]", layerType, layerIndex, layerDesc.neuronCount, connectionCount).c_str());
		else if (m_LayerProps[layerIndex].collapsed == false)
			drawList->AddText(ImGui::GetFont(), textSize * m_CanvasCamera.zoom, m_CanvasCamera.WorldToScreen(headerCursorPos), col, std::format("[{}, {}]", layerDesc.neuronCount, connectionCount).c_str());

		drawList->AddLine(m_CanvasCamera.WorldToScreen(ImVec2(xPos, yBegin + headerHeight)), m_CanvasCamera.WorldToScreen(ImVec2(xPos + visibleColumns * horizontalNeuronSpacing, yBegin + headerHeight)), col, 2.0f * m_CanvasCamera.zoom);

		yBegin += headerHeight;

		layerWidth += horizontalNeuronSpacing * visibleColumns;


		if (connectionCount > maxVerticalNeuronCount)
			connectionCount = std::ceil(std::sqrt((float)connectionCount));

		float conYBegin = headerHeight + -((connectionCount * verticalNeuronSpacing) * 0.5f) + verticalNeuronSpacing * 0.5f;

		for (size_t i = 0; i < rows; i++) {

			float rectX = 0;
			float rectStep = neuronSize * 2 / (float)columns;

			for (size_t j = 0; j < columns; j++) {
				if (i * columns + j >= layerDesc.neuronCount)
					break;
				
				if (m_LayerProps[layerIndex].collapsed && j > 0)
					break;

				float xPosOffset = j * horizontalNeuronSpacing + horizontalNeuronSpacing * 0.5f;
				float yPosOffset = verticalNeuronSpacing * 0.5f;
				float yPos = yBegin + i * verticalNeuronSpacing;
				ImVec2 worldPos = m_CanvasCamera.WorldToScreen(ImVec2(xPos + xPosOffset, yPos + yPosOffset));
				drawList->AddCircleFilled(worldPos, neuronSize * m_CanvasCamera.zoom, col);

				drawList->AddCircleFilled(worldPos, (neuronSize - 2.0f) * m_CanvasCamera.zoom, canvasBgColor);

				ImColor fillColor = ImColor(col);
				fillColor.Value.w = m_CurrentModel.getNeuronData(layerIndex, (m_VisualizedBatchIndex * layerDesc.neuronCount) + i * columns + j);
				drawList->AddCircleFilled(worldPos, (neuronSize - 2.0f) * m_CanvasCamera.zoom, fillColor);

				if (neuronSize * m_CanvasCamera.zoom > 9.0f) {
					switch (layerDesc.activation) {
					case ActivationFunction::ReLU:
						drawCurve<24>(worldPos, neuronSize * m_CanvasCamera.zoom, m_CanvasCamera.zoom, g_ReLUCurve, col);
						break;
					case ActivationFunction::SiLU:
						drawCurve<24>(worldPos, neuronSize * m_CanvasCamera.zoom, m_CanvasCamera.zoom, g_SiLUCurve, col);
						break;
					default:
						break;
					}
				}
			}



			float yPos = yBegin + (verticalNeuronSpacing * 0.5f) + i * verticalNeuronSpacing;

			ImVec2 worldPos = m_CanvasCamera.WorldToScreen(ImVec2(xPos + horizontalNeuronSpacing * visibleColumns, yPos));

			float minWeight = 0.0f, maxWeight = 1.0f;

			if (layerDesc.type != LayerType::Output) {
				minWeight = m_CurrentModel.getLayerMinWeight(layerIndex + 1);
				maxWeight = m_CurrentModel.getLayerMaxWeight(layerIndex + 1);
			}
			
			for (size_t j = 0; j < connectionCount; j++) {
				float connectionY = conYBegin + j * neuronSize * 3;


				float summedWeight = 0.0f;
				for (size_t k = 0; k < columns; k++) {
					summedWeight += m_CurrentModel.getLayerWeights(layerIndex + 1)[(i * columns + k) * connectionCount + j];
				}

				float averageWeight = summedWeight / (float)columns;

				float normalizedWeight = (averageWeight - minWeight) / (maxWeight - minWeight);

				ImColor connectionColor = col;
				connectionColor.Value.w = normalizedWeight;

				drawList->AddLine(worldPos, m_CanvasCamera.WorldToScreen(ImVec2(xPos + horizontalNeuronSpacing * visibleColumns + layerSpacing, connectionY)), connectionColor, 2.0f * m_CanvasCamera.zoom);
			}	

			if (layerDesc.type == LayerType::Output) {
				std::string outputText;

				if (layerDesc.activation == ActivationFunction::Softmax)
					outputText =  std::format("{}: [{:.2f}%]", i, m_CurrentModel.getNeuronData(layerIndex, (m_VisualizedBatchIndex * layerDesc.neuronCount) + i* columns + 0) * 100);
				else 
					outputText = std::format("{}: [{:.2f}]", i, m_CurrentModel.getNeuronData(layerIndex, (m_VisualizedBatchIndex * layerDesc.neuronCount) + i * columns + 0));
				drawList->AddText(ImGui::GetFont(), textSizeScaled, m_CanvasCamera.WorldToScreen(ImVec2(xPos + horizontalNeuronSpacing * visibleColumns + style.ItemInnerSpacing.x, yPos - textSize * 0.5f)), col, outputText.c_str());

			}

		}
	}
	
	float ModelEditor::calculateTotalNetworkWidth() {
		float totalWidth = 0.0f;

		for (size_t i = 0; i < m_CurrentModel.getLayerCount(); ++i) {
			const LayerDesc& layerDesc = m_CurrentModel.getLayerDescription(i);

			float horizontalLayerCount = std::ceil(sqrt((float)layerDesc.neuronCount));

			if(m_LayerProps[i].collapsed)
				horizontalLayerCount = 1;

			float  layerWidth = (horizontalLayerCount * horizontalNeuronSpacing);
			totalWidth += layerWidth + layerSpacing;
		}

		return totalWidth - layerSpacing;
	}

	void ModelEditor::drawCanvasControls() {

		ImVec2 canvasSize = ImGui::GetItemRectSize();
		ImVec2 canvasMin = ImGui::GetItemRectMin();
		ImVec2 canvasMax = ImGui::GetItemRectMax();

		ImGuiStyle& style = ImGui::GetStyle();

		float toolbarWidth = 160.0f;
		float toolbarHeight = style.WindowPadding.y * 2.0f + style.FramePadding.y * 2.0f + ImGui::CalcTextSize("Fit").y;
		float toolbarPadding = style.WindowPadding.x;

		ImGui::SetCursorScreenPos(ImVec2(canvasMax.x - toolbarWidth - toolbarPadding, canvasMin.y + toolbarPadding));

		ImGui::BeginChild("CanvasControls", ImVec2(toolbarWidth, toolbarHeight), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);

		ImGui::PushItemWidth(60.0f);

		if(ImGui::Button("Fit")) {
			constexpr float padding = 30.0f;

			float modelWidth = calculateTotalNetworkWidth();
			m_CanvasCamera.zoom = std::clamp((canvasSize.x - 2 * padding) / modelWidth, 0.1f, 10.0f);

			m_CanvasCamera.offset.x = 0;
			m_CanvasCamera.offset.y = 0;
		}

		ImGui::SameLine();
		ImGui::DragInt("Batch", &m_VisualizedBatchIndex, 1, 0, m_BatchSize - 1);
		ImGui::PopItemWidth();


		ImGui::EndChild();

	}

	void ModelEditor::drawCanvasGrid() {
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		float gridSpacing = 30.0f;
		while (gridSpacing * m_CanvasCamera.zoom < 20.0f)
			gridSpacing *= 2.0f;

		const float zoom = m_CanvasCamera.zoom;

		ImVec2 worldMin = m_CanvasCamera.ScreenToWorld(ImGui::GetItemRectMin());
		ImVec2 worldMax = m_CanvasCamera.ScreenToWorld(ImGui::GetItemRectMax());

		const float startX = std::floor(worldMin.x / gridSpacing) * gridSpacing;
		const float startY = std::floor(worldMin.y / gridSpacing) * gridSpacing;

		ImU32 gridColor = IM_COL32(90, 90, 90, 90);

		for (float x = startX; x <= worldMax.x; x += gridSpacing) {
			for (float y = startY; y <= worldMax.y; y += gridSpacing) {
				drawList->AddCircleFilled(m_CanvasCamera.WorldToScreen(ImVec2(x, y)), 1.5f, gridColor);
			}
		}

	}


	void ModelEditor::drawModelVisualization() {

		ImGui::Begin("Model Visualization");

		ImGui::InvisibleButton("CanvasInteraction",	ImVec2(-1, -1), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImVec2 mousePos = ImGui::GetMousePos();

		const ImU32 canvasBgColor = ImGui::GetColorU32(ImGuiCol_MenuBarBg);
		const ImVec2 canvasSize = ImGui::GetItemRectSize();
		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 max = ImGui::GetItemRectMax();

		m_CanvasCamera.canvasCenter.x = (min.x + max.x) * 0.5f;
		m_CanvasCamera.canvasCenter.y = (min.y + max.y) * 0.5f;

		drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest, nullptr);
		drawList->PushClipRect(min, max);
		drawList->AddRectFilled(min, max, canvasBgColor);

		if (m_Training) {
			std::string trainInfo = std::format("Training Epoch: {} / {}, Step: {} / {}, Time per Step: {:.2f}ms", m_CurrentEpoch, m_TrainingEpochs, m_EpochStep, m_CurrentDataSet.getDataCount() / m_BatchSize, m_StepTime * 1000.0);
			drawList->AddText(ImGui::GetFont(), 20.0f, min, 0xFFFFFFFF, trainInfo.c_str());
			std::string currentLossInfo = std::format("Current Loss: {:.7f}", m_CurrentModel.getCurrentLoss());
			drawList->AddText(ImGui::GetFont(), 20.0f, ImVec2(min.x, min.y + 20.0f), 0xFFFFFFFF, currentLossInfo.c_str());

			std::string accuracyInfo = std::format("Test Accuracy: {:.2f}%", accuracy * 100.0f);
			drawList->AddText(ImGui::GetFont(), 20.0f, ImVec2(min.x, min.y + 40.0f), 0xFFFFFFFF, accuracyInfo.c_str());
		}


		
		ImVec2 canvasMiddleImGUiSpace;
		canvasMiddleImGUiSpace.x = (m_VisualizationX * m_VisualizationScale) + (canvasSize.x / 2) + min.x;
		canvasMiddleImGUiSpace.y = (m_VisualizationY * m_VisualizationScale) + (canvasSize.y / 2) + min.y;

		drawCanvasGrid();

		float xBegin = 0-(calculateTotalNetworkWidth() / 2);
		
		for (size_t i = 0; i < m_CurrentModel.getLayerCount(); ++i) {
			const LayerDesc& desc = m_CurrentModel.getLayerDescription(i);
			LayerDesc nextDesc;
			if (desc.type != LayerType::Output) {
				nextDesc = m_CurrentModel.getLayerDescription(i + 1);
			}

			drawLayerVisualization(xBegin, i, xBegin);

			xBegin += layerSpacing;
		}

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
			m_CanvasCamera.offset.x += ImGui::GetIO().MouseDelta.x;
			m_CanvasCamera.offset.y += ImGui::GetIO().MouseDelta.y;
		}

		if (ImGui::IsItemHovered()) {
			float wheel = ImGui::GetIO().MouseWheel;
			if (wheel != 0.0f) {
				ImVec2 mousePos = ImGui::GetIO().MousePos;
				ImVec2 mouseWorldPos = m_CanvasCamera.ScreenToWorld(mousePos);

				m_CanvasCamera.zoom = std::clamp(m_CanvasCamera.zoom * std::pow(1.1f, wheel), 0.1f, 10.0f);

				printf("Zoom: %f\n", m_CanvasCamera.zoom);

				m_CanvasCamera.offset.x = mousePos.x - m_CanvasCamera.canvasCenter.x - mouseWorldPos.x * m_CanvasCamera.zoom;
				m_CanvasCamera.offset.y = mousePos.y - m_CanvasCamera.canvasCenter.y - mouseWorldPos.y * m_CanvasCamera.zoom;

				ImVec2 mouseWorldPosAfterZoom = m_CanvasCamera.ScreenToWorld(mousePos);
				printf("Before {%f, %f}, After {%f, %f}\n", mouseWorldPos.x, mouseWorldPos.y, mouseWorldPosAfterZoom.x, mouseWorldPosAfterZoom.y);

			}
		}

		drawList->PopClipRect();
		drawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);
		drawCanvasControls();

		ImGui::End();

	}

	void ModelEditor::drawTrainingPanel()
	{
		const ImGuiStyle& style = ImGui::GetStyle();


		ImGui::Begin("Training");
		ImGui::AlignTextToFramePadding();

		if (m_Training) ImGui::BeginDisabled();

		ImGui::Text("Batch Size:");

		ImGui::SameLine();

		ImGui::SetNextItemWidth(100.0f);
		ImGui::DragInt("##BatchSize", &m_BatchSize, 1, 1, 1024);

		ImGui::SameLine();

		if (ImGui::Button("Initialize Params")) {
			m_CurrentModel.buildModel(m_BatchSize);
			m_CurrentModel.initializeParameters();
			m_CurrentModel.beginTraining();
		}

		ImGui::Separator();

		ImGui::Text("Dataset Settings");

		ImGui::Text("%s", m_LoadedDataSet.c_str());

		static bool newSelected;

		if (ImGui::BeginCombo("Dataset", m_LoadedDataSet.c_str())) {
			std::filesystem::path datasetFolder = "DataSets";
				
			if (!std::filesystem::exists(datasetFolder)) {
				std::filesystem::create_directories(datasetFolder);
			}

			for (const auto& entry : std::filesystem::directory_iterator(datasetFolder)) {
				if (entry.is_regular_file()) {
					const auto& path = entry.path();
					std::string label = std::format("{}", path.filename().string().c_str());
					
					bool selected = m_LoadedDataSet.compare(label) == 0;
					ImGui::Selectable(label.c_str(), &selected);
					if (selected) m_LoadedDataSet = label;

				}
			}
			ImGui::Selectable("New", &newSelected);
			ImGui::EndCombo();
		}

		if (newSelected) {
			std::string name = "New DataSet";
			ImGui::InputText("DataSet Name", &name);

		}

		ImGui::Separator();

		ImGui::InputInt("Epochs", &m_TrainingEpochs);

		ImGui::Separator();
		ImGui::Text("Optimizer Settings");

		int optimizer = static_cast<int>(m_CurrentModel.getOptimizer());
		ImGui::Combo("Optimizer", &optimizer, "SGD\0AdamW\0");

		if (optimizer == OptimizerType::SGD) {
			Math::SGDOptimzierParams sgdParams = m_CurrentModel.getSGDParams();
			ImGui::InputFloat("Learning Rate", &sgdParams.learningRate, 0.0001f, 0.01f, "%.5f");
			m_CurrentModel.setOptimizer(sgdParams);
		}
		else if (optimizer == OptimizerType::AdamW) {
			ImGui::PushItemWidth(100.0f);
			Math::AdamOptimizerParams adamParams = m_CurrentModel.getAdamParams();
			ImGui::InputFloat("Learning Rate", &adamParams.learningRate, 0.0001f, 0.01f, "%.9f");
			ImGui::InputFloat("Beta 1", &adamParams.beta1, 0.0001f, 0.01f, "%.9f");
			ImGui::InputFloat("Beta 2", &adamParams.beta2, 0.0001f, 0.01f, "%.9f");
			ImGui::InputFloat("Epsilon", &adamParams.epsilon, 0.0001f, 0.01f, "%.9f");
			ImGui::InputFloat("Weight Decay", &adamParams.weightDecay, 0.0001f, 0.01f, "%.9f");
			m_CurrentModel.setOptimizer(adamParams);
			ImGui::PopItemWidth();
		}

		ImGui::Separator();

		if (m_Training) ImGui::EndDisabled();
		
		if (ImGui::Button("Train")) m_Training = true;

		if (m_Training) {
			ImGui::SameLine();
		
			if (ImGui::Button(m_TrainingPaused ? "Resume" : "Pause")) m_TrainingPaused = !m_TrainingPaused;
			ImGui::SameLine();
			if (ImGui::Button("Stop")) m_Training = false;


			if (ImGui::Button("Save Model")) {
				std::filesystem::path modelDirectory = "models";
				if (!std::filesystem::exists(modelDirectory)) {
					std::filesystem::create_directories(modelDirectory);
				}
				std::string modelName = std::format("mnist_epoch_{}.modl", m_TrainingEpochs);
				std::filesystem::path modelPath = modelDirectory / modelName;
				m_CurrentModel.saveModel(modelPath);
			}

			ImGui::SameLine();

		}



		ImGui::End();
	}


}

