#pragma once

#include "Model.h"
#include "DataSet.h"
#include "external/ImGui/imgui.h"

namespace AI {
	class ModelEditor {
	public:
		ModelEditor();

		void update(float deltaTime);

		void drawGUI();
		void render();

	private:

		void drawModelBrowser();
		void drawModelParameters();
		void drawModelVisualization();
		void drawTrainingPanel();

		void drawLayerParameters(size_t layerIndex);

		void drawCanvasControls();
		void drawCanvasGrid();
		void drawLayerVisualization(float xPos, uint32_t layerIndex, float& layerWidth);

		float calculateTotalNetworkWidth();

		struct CanvasCamera {
			ImVec2 offset = { 0.0f, 0.0f};
			float zoom = 1.0f;
	
			ImVec2 canvasCenter = { 0.0f, 0.0f };

			inline ImVec2 WorldToScreen(ImVec2 worldPos)  {
				return { worldPos.x * zoom + canvasCenter.x + offset.x, worldPos.y * zoom + canvasCenter.y + offset.y };
			}

			inline ImVec2 ScreenToWorld(ImVec2 screenPos)  {
				return  { (screenPos.x - canvasCenter.x - offset.x) / zoom, (screenPos.y - canvasCenter.y - offset.y) / zoom };
			}

		} m_CanvasCamera;

		float m_VisualizationScale = 1.0f;
		float m_VisualizationX = 0.0f;
		float m_VisualizationY = 0.0f;

		Model m_CurrentModel;

		DataSet m_CurrentDataSet;

		struct LayerProperties {
			bool collapsed = false;
		};

		std::vector<LayerProperties> m_LayerProps;

		int m_VisualizedBatchIndex = 0;

		int m_BatchSize = 1;
		int m_TrainingEpochs = 20;


		bool m_Training = false;
		bool m_TrainingPaused = false;
		int m_CurrentEpoch = 0;

		size_t m_EpochStep = 0;

		double m_StepTime = 0.01;

		std::string m_LoadedDataSet;

	};
}