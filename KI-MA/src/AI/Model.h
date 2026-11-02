#pragma once
#include "Tensor.h"
#include "Math/TensorMath.h"
#include "Math/OptimizerMath.h"
#include <vector>
#include <filesystem>



namespace AI {

	enum LayerType {
		Input,
		Dense,
		Output,
		Input2D,
		Conv2D,
		Flatten,
	};

	enum ActivationFunction {
		None,
		ReLU,
		SiLU,
		Softmax,
	};

	enum OptimizerType {
		SGD,
		AdamW,
	};

	struct LayerDesc {
		LayerType type;
		ActivationFunction activation;
		size_t neuronCount;

		size_t inputHeight = 0;
		size_t inputWidth = 0;
		size_t inputChannels = 0;

		// Conv2D parameters
		size_t kernelHeight = 0;
		size_t kernelWidth = 0;
		size_t strideHeight = 1;
		size_t strideWidth = 1;
		size_t paddingHeight = 0;
		size_t paddingWidth = 0;
		size_t filterCount = 0;

		size_t layerDataIndex = 0;
	};


	class Model {
	public:
		Model() {};
		~Model() {};

		void buildModel(size_t batchSize = 1);

		void initializeParameters();

		void forward(Tensor<float>& input, Tensor<float>& output);
		void forwardGPU(Tensor<float>& input, Tensor<float>& output);

		void beginTraining();
		void endTraining();

		void setOptimizer(const Math::SGDOptimzierParams& sgdParams) { m_SGDParams = sgdParams; m_OptimizerType = OptimizerType::SGD; };
		void setOptimizer(const Math::AdamOptimizerParams& adamParams) { m_AdamParams = adamParams; m_OptimizerType = OptimizerType::AdamW; };

		Math::SGDOptimzierParams getSGDParams() { return m_SGDParams; }
		Math::AdamOptimizerParams getAdamParams() { return m_AdamParams; }

		OptimizerType getOptimizer() { return m_OptimizerType; }

		void trainSupervised(Tensor<float>& target);
		void trainSupervisedGPU(Tensor<float>& target);
		float getCurrentLoss() const;

		void saveModel(const std::filesystem::path& filename);
		bool loadModel(const std::filesystem::path& filename, size_t batchSize);

		void destroyModel();

		const LayerDesc& getLayerDescription(size_t index) const {
			return m_LayerDescription[index];
		}

		void addLayer(LayerType type, ActivationFunction activation, size_t inputSize) {
			m_LayerDescription.push_back({ type, activation, inputSize, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
		}

		void addLayer(LayerType type, ActivationFunction activation, size_t inputHeight, size_t inputWidth, size_t inputChannels) {
			m_LayerDescription.push_back({ type, activation, 0, inputHeight, inputWidth, inputChannels, 0, 0, 0, 0, 0, 0, 0 });
		}

		void addLayer(LayerType type, ActivationFunction activation, size_t kernelHeight, size_t kernelWidth, size_t strideHeight, size_t strideWidth, size_t paddingHeight, size_t paddingWidth, size_t filterCount) {
			m_LayerDescription.push_back({ type, activation, 0, 0, 0, 0, kernelHeight, kernelWidth, strideHeight, strideWidth, paddingHeight, paddingWidth, filterCount });
		}

		void addLayer(LayerType type, ActivationFunction activation) {
			m_LayerDescription.push_back({ type, activation, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
		}

		void removeLayer(size_t index) {
			m_LayerData.erase(m_LayerData.begin() + index);
		}

		size_t getLayerCount() const {
			return m_LayerDescription.size();
		}

		void clearLayers() {
			m_LayerDescription.clear();
		}

		void setLayer(size_t layerIndex, const Tensor<float>& weights, const Tensor<float>& biases) {
			m_LayerData[m_LayerDescription[layerIndex].layerDataIndex].weights = weights;
			m_LayerData[m_LayerDescription[layerIndex].layerDataIndex].biases = biases;
		}

		const Tensor<float>& getLayerWeights(size_t layerIndex) const {
			return m_LayerData[m_LayerDescription[layerIndex].layerDataIndex].weights;
		}

		const Tensor<float>& getLayerBiases(size_t layerIndex) const {
			return m_LayerData[m_LayerDescription[layerIndex].layerDataIndex].biases;
		}

		const float& getLayerWeight(size_t layerIndex, size_t weightIndex) const {
			return m_LayerData[m_LayerDescription[layerIndex].layerDataIndex].weights[weightIndex];
		}

		const float getLayerMinWeight(size_t layerIndex) {
			return m_LayerData[m_LayerDescription[layerIndex].layerDataIndex].weights.getMinValue();
		}

		const float getLayerMaxWeight(size_t layerIndex) {
			return m_LayerData[m_LayerDescription[layerIndex].layerDataIndex].weights.getMaxValue();
		}

		const float getNeuronData(size_t layerIndex, size_t neuronIndex) const {
			if (layerIndex == 0) return m_SavedInputValues[neuronIndex];
			return m_LayerData[m_LayerDescription[layerIndex].layerDataIndex].activationOutput[neuronIndex];
		}

	private:
		//void SGDUpdate(Tensor<float>& param, Tensor<float>& grad);
		void AdamWUpdate(Tensor<float>& param, Tensor<float>& grad, Math::AdamOptimizerState& state, Math::AdamOptimizerParams& params, float correction1, float correction2, bool decay);

		struct LayerData {
			Tensor<float> weights;
			Tensor<float> biases;

			Math::BroadcastPlan weightBiasPlan;
			Math::BroadcastPlan inputWeightPlan;

			// Forwad Daten
			Tensor<float> matMulOutput;
			Tensor<float> addOutput;

			Tensor<float> preActivation;
			Tensor<float> activationOutput;

			Tensor<float> im2colOutput;

			// Training Daten
			Tensor<float> transposedLayerValues;
			Tensor<float> weightGradient;
			Tensor<float> biasGradient;
			Tensor<float> transposedWeights;
			Tensor<float> inputGradient;

			Math::BroadcastPlan weightGradientPlan;
			Math::BroadcastPlan inputGradientPlan;

			Tensor<float> im2colGradient;

			std::vector<size_t> inputShape;
			std::vector<size_t> outputShape;
		};

		bool m_ModelReady = false;
		std::vector<LayerDesc> m_LayerDescription;

		std::vector<LayerData> m_LayerData;
		
		// Training Daten
		bool m_Training = false;
		size_t m_BatchSize = 1;

		Tensor<float> m_SavedInputValues;

		Tensor<float> m_LossGradient;
		Tensor<float> m_LastTarget;
		size_t m_TrainStep = 0;



		//Optimizer
		OptimizerType m_OptimizerType = OptimizerType::SGD;
		Math::SGDOptimzierParams m_SGDParams;
		Math::AdamOptimizerParams m_AdamParams;

		std::vector<Math::AdamOptimizerState> m_AdamStates;
		std::vector<Math::AdamOptimizerState> m_AdamStatesBias;
		float m_AdamCorrection1 = 1.0f;
		float m_AdamCorrection2 = 1.0f;
	};
}