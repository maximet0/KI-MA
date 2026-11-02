#include "Model.h"

#include <random>


#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <algorithm>

#include "Math/GPUTensorMath.h"

namespace AI {
	
	void Model::buildModel(size_t batchSize) {
		m_LayerData.clear();
		m_BatchSize = batchSize;

		if (m_LayerDescription.size() < 2) {
			Core::Logger::Fatal("Model must have at least 2 layers (input and output)");
		}

		m_LayerData.resize(m_LayerDescription.size() - 1);

		size_t layerDataIndex = 0;

		std::vector<size_t> currentShape;

		// Erster Layer ist Input Layer
		for (size_t i = 0; i < m_LayerDescription.size(); i++) {
			if (i == 0 && m_LayerDescription[i].type != LayerType::Input && m_LayerDescription[i].type != LayerType::Input2D) {
				Core::Logger::Fatal("First layer must be an input layer");
	
			}
			else {
				if (m_LayerDescription[i].type == LayerType::Input2D) {
					m_SavedInputValues = Tensor<float>({ m_BatchSize, m_LayerDescription[i].inputHeight, m_LayerDescription[i].inputWidth, m_LayerDescription[i].inputChannels });
					currentShape = { m_LayerDescription[i].inputHeight, m_LayerDescription[i].inputWidth, m_LayerDescription[i].inputChannels };
				}
				else if (m_LayerDescription[i].type == LayerType::Input) {
					m_SavedInputValues = Tensor<float>({ m_BatchSize, m_LayerDescription[i].neuronCount });
					currentShape = { m_LayerDescription[i].neuronCount };
				}

			}

			const LayerDesc& desc = m_LayerDescription[i];

			if (i < m_LayerDescription.size() - 1) {
				const LayerDesc& nextDesc = m_LayerDescription[i + 1];
				LayerData& layerData = m_LayerData[layerDataIndex];
				m_LayerDescription[i + 1].layerDataIndex = layerDataIndex;

				if (nextDesc.type == LayerType::Dense || nextDesc.type == LayerType::Output) {
					// Weights [inputs, neurons]
					size_t inputNeurons = currentShape[0];

					layerData.weights = Tensor<float>({ inputNeurons, nextDesc.neuronCount });
					layerData.biases = Tensor<float>({ 1, nextDesc.neuronCount });
					Tensor<float> inputTensor({ m_BatchSize, inputNeurons });
					layerData.inputWeightPlan = Math::CreateBroadcastPlan({ &inputTensor, &layerData.weights });

					Tensor<float> outputTensor({ m_BatchSize, nextDesc.neuronCount });
					layerData.weightBiasPlan = Math::CreateBroadcastPlan({ &outputTensor, &layerData.biases });


					layerData.matMulOutput = Tensor<float>({ m_BatchSize, nextDesc.neuronCount });
					layerData.addOutput = Tensor<float>({ m_BatchSize, nextDesc.neuronCount });
					layerData.preActivation = Tensor<float>({ m_BatchSize, nextDesc.neuronCount });
					layerData.activationOutput = Tensor<float>({ m_BatchSize, nextDesc.neuronCount });


					layerData.inputShape = currentShape;
					layerData.outputShape = { nextDesc.neuronCount };

					currentShape = layerData.outputShape;

					layerDataIndex++;
				}
				else if (nextDesc.type == LayerType::Conv2D) {

					size_t H = currentShape[0];
					size_t W = currentShape[1];
					size_t C = currentShape[2];

					size_t outHeight = (H + 2 * nextDesc.paddingHeight - nextDesc.kernelHeight) / nextDesc.strideHeight + 1;
					size_t outWidth = (W + 2 * nextDesc.paddingWidth - nextDesc.kernelWidth) / nextDesc.strideWidth + 1;

					size_t M = m_BatchSize * outHeight * outWidth;
					size_t K = nextDesc.kernelHeight * nextDesc.kernelWidth * C;

					layerData.weights = Tensor<float>({ K, nextDesc.filterCount });
					layerData.biases = Tensor<float>({ 1, nextDesc.filterCount });

					layerData.im2colOutput = Tensor<float>({ M, K });

					layerData.inputWeightPlan = Math::CreateBroadcastPlan({ &layerData.im2colOutput, &layerData.weights });

					Tensor<float> outputTensor({ M, nextDesc.filterCount });
					layerData.weightBiasPlan = Math::CreateBroadcastPlan({ &outputTensor, &layerData.biases });

					layerData.matMulOutput = Tensor<float>({ M, nextDesc.filterCount });
					layerData.addOutput = Tensor<float>({ M, nextDesc.filterCount });
					layerData.preActivation = Tensor<float>({ M, nextDesc.filterCount });
					layerData.activationOutput = Tensor<float>({ M, nextDesc.filterCount });

					layerData.inputShape = { H, W, C };
					layerData.outputShape = { outHeight, outWidth, nextDesc.filterCount };
					currentShape = layerData.outputShape;
					layerDataIndex++;
				}
				else if (nextDesc.type == LayerType::Flatten) {
					size_t features = currentShape[0] * currentShape[1] * currentShape[2];

					layerData.inputShape = currentShape;
					layerData.outputShape = { features };
					layerData.activationOutput = Tensor<float>({ m_BatchSize, features });
					currentShape = { features };
					layerDataIndex++;
				}
			}
		}
		m_ModelReady = true;
	}
	
	void Model::initializeParameters() {
		static std::mt19937 rng(1);
		size_t index = 1;
		for (auto& layerData : m_LayerData) {
			// Biases werden 0 initialisiert
			memset(layerData.biases.getDataPtr(), 0, layerData.biases.getTotalSize() * sizeof(float));

			// Weights werden zufällig initialisiert, um Symmetrie zu vermeiden.
			// Die verteilung der Weights hängt von der Aktivierungsfunktion und der Anzahl der Neuronen im Layer ab
			
			std::normal_distribution<float> dist;
			if (m_LayerDescription[index].activation == ActivationFunction::ReLU || m_LayerDescription[index].activation == ActivationFunction::SiLU) {
				// He et al. Initialisierung (v^2 = 2 / n_in) https://arxiv.org/pdf/1502.01852
				float stddev = std::sqrt(2.0f / m_LayerDescription[index - 1].neuronCount);
				dist = std::normal_distribution<float>(0.0f, stddev);
			}
			else if (m_LayerDescription[index].activation == ActivationFunction::None || m_LayerDescription[index].activation == ActivationFunction::Softmax) {
				// Xavier Initialisierung (v^2 = 1 / (n_in + n_out))
				float stddev = std::sqrt(1.0f / (m_LayerDescription[index - 1].neuronCount + m_LayerDescription[index].neuronCount));
				dist = std::normal_distribution<float>(0.0f, stddev);
			}

			for (size_t i = 0; i < layerData.weights.getTotalSize(); ++i)
				layerData.weights[i] = dist(rng);

			index++;
		}
	}

	std::string formatTensor(AI::Tensor<float>& tensor) {
		std::string result = "[";
		for (size_t i = 0; i < tensor.getShape().size(); ++i) {
			result += std::to_string(tensor.getShape()[i]);
			if (i < tensor.getShape().size() - 1) result += ", ";
		}
		result += "], ";
		result += "\nValues:";
		for (size_t i = 0; i < tensor.getTotalSize(); ++i) {
			result += std::to_string(tensor[i]) + " ";
		}
		return result;
	}

	void Model::forward(Tensor<float>& input, Tensor<float>& output) {

		Tensor<float>* curInput = &input;
		curInput->copyTo(m_SavedInputValues);

		for (size_t i = 0; i < m_LayerDescription.size(); i++) {
			const LayerDesc& curLayerDesc = m_LayerDescription[i];
			LayerData& curLayerData = m_LayerData[curLayerDesc.layerDataIndex];

			if (curLayerDesc.type == LayerType::Dense || curLayerDesc.type == LayerType::Output) {
				Tensor<float>& weights = curLayerData.weights;
				Tensor<float>& biases = curLayerData.biases;

				// output = input * weights + biases
				Math::MatMul(*curInput, weights, curLayerData.matMulOutput, curLayerData.inputWeightPlan);
				Math::Add(curLayerData.matMulOutput, biases, curLayerData.preActivation, curLayerData.weightBiasPlan);

				if (curLayerDesc.activation == ActivationFunction::SiLU) {
					Math::SiLU(curLayerData.preActivation, curLayerData.activationOutput);
				}
				else if (curLayerDesc.activation == ActivationFunction::ReLU) {
					Math::ReLU(curLayerData.preActivation, curLayerData.activationOutput);
				}
				else if (curLayerDesc.activation == ActivationFunction::Softmax) {
					Math::Softmax(curLayerData.preActivation, curLayerData.activationOutput);
				}
				else curLayerData.preActivation.copyTo(curLayerData.activationOutput);

				curInput = &curLayerData.activationOutput;
			}
			else if (curLayerDesc.type == LayerType::Conv2D) {
				Tensor<float>& weights = curLayerData.weights;
				Tensor<float>& biases = curLayerData.biases;

				Math::Im2Col(*curInput, curLayerData.im2colOutput, curLayerDesc.kernelHeight, curLayerDesc.kernelWidth, curLayerDesc.strideHeight, curLayerDesc.strideWidth, curLayerDesc.paddingHeight, curLayerDesc.paddingWidth);

				Math::MatMul(curLayerData.im2colOutput, weights, curLayerData.matMulOutput, curLayerData.inputWeightPlan);
				Math::Add(curLayerData.matMulOutput, biases, curLayerData.preActivation, curLayerData.weightBiasPlan);
				
				Core::Logger::Debug("PreActivation: {}", formatTensor(curLayerData.preActivation));

				curLayerData.activationOutput.setShape(curLayerData.preActivation.getShape());

				if (curLayerDesc.activation == ActivationFunction::SiLU) {
					Math::SiLU(curLayerData.preActivation, curLayerData.activationOutput);
				}
				else if (curLayerDesc.activation == ActivationFunction::ReLU) {
					Math::ReLU(curLayerData.preActivation, curLayerData.activationOutput);
				}
				else curLayerData.preActivation.copyTo(curLayerData.activationOutput);
				curLayerData.activationOutput.setShape({ m_BatchSize, curLayerData.outputShape[0], curLayerData.outputShape[1], curLayerDesc.filterCount });
				curInput = &curLayerData.activationOutput;
				
			}
			else if (curLayerDesc.type == LayerType::Flatten) {

				size_t features = curInput->getTotalSize() / m_BatchSize;

				curInput->copyTo(curLayerData.activationOutput);
				curLayerData.activationOutput.setShape({ m_BatchSize, features });
				curInput = &curLayerData.activationOutput;
			}

		}

		curInput->copyTo(output);
	}

	void Model::forwardGPU(Tensor<float>&input, Tensor<float>& output) {

		Tensor<float>* curInput = &input;
		curInput->copyTo(m_SavedInputValues);

		for (size_t i = 0; i < m_LayerDescription.size(); i++) {
			const LayerDesc& curLayerDesc = m_LayerDescription[i];
			LayerData& curLayerData = m_LayerData[curLayerDesc.layerDataIndex];


			if (curLayerDesc.type == LayerType::Dense || curLayerDesc.type == LayerType::Output) {
				Tensor<float>& weights = curLayerData.weights;
				Tensor<float>& biases = curLayerData.biases;

				// output = input * weights + biases
				Math::GPUMatMul(*curInput, weights, curLayerData.matMulOutput, curLayerData.inputWeightPlan);
				Math::GPUAdd(curLayerData.matMulOutput, biases, curLayerData.preActivation, curLayerData.weightBiasPlan);

				if (curLayerDesc.activation == ActivationFunction::SiLU) {
					Math::GPUSiLU(curLayerData.preActivation, curLayerData.activationOutput);
				} else if (curLayerDesc.activation == ActivationFunction::ReLU) {
					Math::GPUReLU(curLayerData.preActivation, curLayerData.activationOutput);
				} else if (curLayerDesc.activation == ActivationFunction::Softmax) {
					Math::GPUSoftmax(curLayerData.preActivation, curLayerData.activationOutput);
				}
				else curLayerData.preActivation.copyTo(curLayerData.activationOutput);

				curInput = &curLayerData.activationOutput;
			}
		}

		curInput->copyTo(output);
		output.markGPUWritten();
	}

	void Model::beginTraining() {
		m_Training = true;
		m_LossGradient = Tensor<float>({ m_BatchSize, m_LayerDescription.back().neuronCount });

		m_LastTarget = Tensor<float>({ m_BatchSize, m_LayerDescription.back().neuronCount });

		if (m_OptimizerType == OptimizerType::AdamW) {
			m_AdamStates.resize(m_LayerData.size());
			m_AdamStatesBias.resize(m_LayerData.size());
			size_t i = 0;
			for (auto& LayerData : m_LayerData) {
				// Initialize Adam states
				m_AdamStates[i].m = Tensor<float>({ LayerData.weights.getShape()[0], LayerData.weights.getShape()[1] });
				m_AdamStates[i].v = Tensor<float>({ LayerData.weights.getShape()[0], LayerData.weights.getShape()[1] });
				m_AdamStatesBias[i].m = Tensor<float>({ 1, LayerData.biases.getShape()[1] });
				m_AdamStatesBias[i].v = Tensor<float>({ 1, LayerData.biases.getShape()[1] });
				std::fill_n(m_AdamStates[i].m.getDataPtr(), m_AdamStates[i].m.getTotalSize(), 0.0f);
				std::fill_n(m_AdamStates[i].v.getDataPtr(), m_AdamStates[i].v.getTotalSize(), 0.0f);
				std::fill_n(m_AdamStatesBias[i].m.getDataPtr(), m_AdamStatesBias[i].m.getTotalSize(), 0.0f);
				std::fill_n(m_AdamStatesBias[i].v.getDataPtr(), m_AdamStatesBias[i].v.getTotalSize(), 0.0f);
				i++;
			}
		}
		
		AI::Tensor<float>* curGradient = &m_LossGradient;

		for (size_t i = m_LayerDescription.size(); i-- > 0; ) {
			const LayerDesc& curLayerDesc = m_LayerDescription[i];
			if (curLayerDesc.type == LayerType::Dense || curLayerDesc.type == LayerType::Output) {
				LayerData& layerData = m_LayerData[curLayerDesc.layerDataIndex];
				layerData.transposedLayerValues = Tensor<float>({ layerData.weights.getShape()[0], m_BatchSize });
				layerData.weightGradient = Tensor<float>({ layerData.weights.getShape()[0], layerData.weights.getShape()[1] });
				layerData.transposedWeights = Tensor<float>({ layerData.weights.getShape()[1], layerData.weights.getShape()[0] });
				layerData.inputGradient = Tensor<float>({ m_BatchSize, layerData.weights.getShape()[0] });
				layerData.biasGradient = Tensor<float>({ 1, layerData.biases.getShape()[1] });
				layerData.weightGradientPlan = Math::CreateBroadcastPlan({ &layerData.transposedLayerValues, curGradient });
				layerData.inputGradientPlan = Math::CreateBroadcastPlan({ curGradient, &layerData.transposedWeights });
				curGradient = &layerData.inputGradient;
			}
		}
	}

	void Model::endTraining() {
		m_Training = false;
	}

	void Model::trainSupervised(Tensor<float>& target) {
		m_TrainStep++;

		target.copyTo(m_LastTarget);

		if (m_LayerDescription.back().activation == ActivationFunction::Softmax) {
			AI::Math::CELossGradient(m_LayerData.back().activationOutput, target, m_LossGradient);
		}
		else {
			AI::Math::MSELossGradient(m_LayerData.back().activationOutput, target, m_LossGradient);
		}

		//Core::Logger::Debug("Current Loss Gradient: {}", formatTensor(m_SavedLayerValues.back()));

		AI::Tensor<float>* curGradient = &m_LossGradient;

		m_AdamCorrection1 = 1.0f - std::pow(m_AdamParams.beta1, static_cast<float>(m_TrainStep));
		m_AdamCorrection2 = 1.0f - std::pow(m_AdamParams.beta2, static_cast<float>(m_TrainStep));

		for (size_t i = m_LayerDescription.size(); i-- > 1; ) {
			const LayerDesc& curLayerDesc = m_LayerDescription[i];
			LayerData& curLayerData = m_LayerData[curLayerDesc.layerDataIndex];
			Tensor<float>& prevActivationOutput = i == 1 ? m_SavedInputValues : m_LayerData[m_LayerDescription[i - 1].layerDataIndex].activationOutput;

			// Berechne den Gradienten der Aktivierungsfunktion
			if (curLayerDesc.activation == ActivationFunction::SiLU) {
				AI::Math::SiLUGradient(curLayerData.preActivation, *curGradient, *curGradient);
			}
			else if (curLayerDesc.activation == ActivationFunction::ReLU) {
				AI::Math::ReLUGradient(curLayerData.preActivation, *curGradient, *curGradient);
			}

			// Berechne den Gradienten der Gewichte und Biases
			if (curLayerDesc.type == LayerType::Dense || curLayerDesc.type == LayerType::Output) {
				AI::Math::Transpose(prevActivationOutput, curLayerData.transposedLayerValues);


				AI::Math::MatMul(curLayerData.transposedLayerValues, *curGradient, curLayerData.weightGradient, curLayerData.weightGradientPlan);

				std::fill_n(curLayerData.biasGradient.getDataPtr(), curLayerData.biasGradient.getTotalSize(), 0.0f);
				for (size_t j = 0; j < m_BatchSize; j++) {
					for (size_t k = 0; k < curLayerData.biasGradient.getTotalSize(); k++) {
						curLayerData.biasGradient[k] += (*curGradient)(j, k);
					}
				}

				//AI::Math::BiasGradient(*curGradient, curLayerData.biasGradient);

				AI::Math::Transpose(curLayerData.weights, curLayerData.transposedWeights);
				AI::Math::MatMul(*curGradient, curLayerData.transposedWeights, curLayerData.inputGradient, curLayerData.inputGradientPlan);


				if (m_OptimizerType == OptimizerType::SGD) {
					AI::Math::SGDUpdate(curLayerData.weights, curLayerData.weightGradient, m_SGDParams);
					AI::Math::SGDUpdate(curLayerData.biases, curLayerData.biasGradient, m_SGDParams);
				}
				else if (m_OptimizerType == OptimizerType::AdamW) {
					AI::Math::AdamWUpdate(curLayerData.weights, curLayerData.weightGradient, m_AdamStates[curLayerDesc.layerDataIndex], m_AdamParams, m_AdamCorrection1, m_AdamCorrection2, true);
					AI::Math::AdamWUpdate(curLayerData.biases, curLayerData.biasGradient, m_AdamStatesBias[curLayerDesc.layerDataIndex], m_AdamParams, m_AdamCorrection1, m_AdamCorrection2, false);
				}

				curLayerData.inputGradient.clearCachedData();
				curGradient = &curLayerData.inputGradient;

			}

		}
	}

	void Model::trainSupervisedGPU(Tensor<float>& target) {
		m_TrainStep++;

		target.copyTo(m_LastTarget);

		if (m_LayerDescription.back().activation == ActivationFunction::Softmax) {
			AI::Math::GPUCELossGradient(m_LayerData.back().activationOutput, target, m_LossGradient);
		}
		else {
			AI::Math::GPUMSELossGradient(m_LayerData.back().activationOutput, target, m_LossGradient);
		}

		AI::Tensor<float>* curGradient = &m_LossGradient;
		
		m_AdamCorrection1 = 1.0f - std::pow(m_AdamParams.beta1, static_cast<float>(m_TrainStep));
		m_AdamCorrection2 = 1.0f - std::pow(m_AdamParams.beta2, static_cast<float>(m_TrainStep));

		for(size_t i = m_LayerDescription.size(); i-- > 1; ) {
			const LayerDesc& curLayerDesc = m_LayerDescription[i];
			LayerData& curLayerData = m_LayerData[curLayerDesc.layerDataIndex];
			Tensor<float>& prevActivationOutput = i == 1 ? m_SavedInputValues : m_LayerData[m_LayerDescription[i - 1].layerDataIndex].activationOutput;

			// Berechne den Gradienten der Aktivierungsfunktion
			if (curLayerDesc.activation == ActivationFunction::SiLU) {
				AI::Math::GPUSiLUGradient(curLayerData.preActivation, *curGradient, *curGradient);
			} else if (curLayerDesc.activation == ActivationFunction::ReLU) {
				AI::Math::GPUReLUGradient(curLayerData.preActivation, *curGradient, *curGradient);
			}

			// Berechne den Gradienten der Gewichte und Biases
			if (curLayerDesc.type == LayerType::Dense || curLayerDesc.type == LayerType::Output) {
				AI::Math::GPUTranspose(prevActivationOutput, curLayerData.transposedLayerValues);

				
				AI::Math::GPUMatMul(curLayerData.transposedLayerValues, *curGradient, curLayerData.weightGradient, curLayerData.weightGradientPlan);
				
				AI::Math::GPUBiasGradient(*curGradient, curLayerData.biasGradient);

				AI::Math::GPUTranspose(curLayerData.weights, curLayerData.transposedWeights);
				AI::Math::GPUMatMul(*curGradient, curLayerData.transposedWeights, curLayerData.inputGradient, curLayerData.inputGradientPlan);


				if (m_OptimizerType == OptimizerType::SGD) {
					AI::Math::GPUSGDUpdate(curLayerData.weights, curLayerData.weightGradient, m_SGDParams);
					AI::Math::GPUSGDUpdate(curLayerData.biases, curLayerData.biasGradient, m_SGDParams);
				}
				else if (m_OptimizerType == OptimizerType::AdamW) {
					AI::Math::GPUAdamWUpdate(curLayerData.weights, curLayerData.weightGradient, m_AdamStates[i - 1], m_AdamParams, m_AdamCorrection1, m_AdamCorrection2, true);
					AI::Math::GPUAdamWUpdate(curLayerData.biases, curLayerData.biasGradient, m_AdamStatesBias[i - 1], m_AdamParams, m_AdamCorrection1, m_AdamCorrection2, false);
				}
			
				curLayerData.inputGradient.clearCachedData();
				curGradient = &curLayerData.inputGradient;

			}

		}

	}

	float Model::getCurrentLoss() const
	{
		float loss = 0.0f;

		if (m_LayerDescription.back().activation == ActivationFunction::Softmax) {
			Math::CELoss(m_LayerData.back().activationOutput, m_LastTarget, loss);
		}
		else {
			Math::MSELoss(m_LayerData.back().activationOutput, m_LastTarget, loss);
		}

		return loss;
	}

	struct modlHDR {
		char magic[4] = { '\0', '\0', '\0', '\0' };
		uint32_t optimizerType = 0;
		uint32_t checkpointStep = 0;
		uint32_t layerCount = 0;
	};

	void Model::saveModel(const std::filesystem::path& filename) {
		std::ofstream file(filename, std::ios::binary);

		modlHDR header;
		header.magic[0] = 'M';
		header.magic[1] = 'O';
		header.magic[2] = 'D';
		header.magic[3] = 'L';
		header.optimizerType = static_cast<uint32_t>(m_OptimizerType);
		header.checkpointStep = static_cast<uint32_t>(m_TrainStep);
		header.layerCount = static_cast<uint32_t>(m_LayerDescription.size());

		file.write(reinterpret_cast<const char*>(&header), sizeof(header));

		if (m_OptimizerType == OptimizerType::SGD) {
			file.write(reinterpret_cast<const char*>(&m_SGDParams), sizeof(Math::SGDOptimzierParams));
		}
		else if (m_OptimizerType == OptimizerType::AdamW) {
			file.write(reinterpret_cast<const char*>(&m_AdamParams), sizeof(Math::AdamOptimizerParams));
		}

		for (size_t i = 0; i < m_LayerDescription.size(); i++) {
			const LayerDesc& desc = m_LayerDescription[i];
			file.write(reinterpret_cast<const char*>(&desc.type), sizeof(uint32_t));
			file.write(reinterpret_cast<const char*>(&desc.activation), sizeof(uint32_t));
			file.write(reinterpret_cast<const char*>(&desc.neuronCount), sizeof(uint32_t));
		}

		for (size_t i = 0; i < m_LayerDescription.size(); i++) {
			const LayerDesc& desc = m_LayerDescription[i];
			if (desc.type == LayerType::Dense || desc.type == LayerType::Output) {
				const LayerData& layerData = m_LayerData[desc.layerDataIndex];
				file.write(reinterpret_cast<const char*>(layerData.weights.getDataPtr()), layerData.weights.getTotalSize() * sizeof(float));
				file.write(reinterpret_cast<const char*>(layerData.biases.getDataPtr()), layerData.biases.getTotalSize() * sizeof(float));
			}
		}

		file.close();
	}

	bool Model::loadModel(const std::filesystem::path & filename, size_t batchSize) {
		destroyModel();
		std::ifstream file(filename, std::ios::binary);
		modlHDR modlHeader;
		file.read(reinterpret_cast<char*>(&modlHeader), sizeof(modlHeader));

		if (modlHeader.magic[0] != 'M' || modlHeader.magic[1] != 'O' || modlHeader.magic[2] != 'D' || modlHeader.magic[3] != 'L') {
			Core::Logger::Error("Invalid model file format, or File doesn't exist.");
			return false;
		}

		
		m_OptimizerType = static_cast<OptimizerType>(modlHeader.optimizerType);
		m_TrainStep = modlHeader.checkpointStep;

		if (m_OptimizerType == OptimizerType::SGD) {
			file.read(reinterpret_cast<char*>(&m_SGDParams), sizeof(Math::SGDOptimzierParams));
		}
		else if (m_OptimizerType == OptimizerType::AdamW) {
			file.read(reinterpret_cast<char*>(&m_AdamParams), sizeof(Math::AdamOptimizerParams));
		}



		uint32_t layerCount = modlHeader.layerCount;

		for(size_t i = 0; i < layerCount; i++) {
			LayerType layerType;
			ActivationFunction activation;
			uint32_t neuronCount;
			file.read(reinterpret_cast<char*>(&layerType), sizeof(LayerType));
			file.read(reinterpret_cast<char*>(&activation), sizeof(ActivationFunction));
			file.read(reinterpret_cast<char*>(&neuronCount), sizeof(uint32_t));

			if (i == 0 && layerType != static_cast<uint32_t>(LayerType::Input)) {
				Core::Logger::Error("First layer must be an input layer");
				return false;
			}

			addLayer(static_cast<LayerType>(layerType), static_cast<ActivationFunction>(activation), neuronCount);
		}

		buildModel(batchSize);

		for (size_t i = 0; i < layerCount; i++) {
			LayerDesc& desc = m_LayerDescription[i];
			if (desc.type == LayerType::Dense || desc.type == LayerType::Output) {
				file.read(reinterpret_cast<char*>(m_LayerData[desc.layerDataIndex].weights.getDataPtr()), m_LayerData[desc.layerDataIndex].weights.getTotalSize() * sizeof(float));
				file.read(reinterpret_cast<char*>(m_LayerData[desc.layerDataIndex].biases.getDataPtr()), m_LayerData[desc.layerDataIndex].biases.getTotalSize() * sizeof(float));
			}
		}

		return true;
	}

	void Model::destroyModel(){
		m_ModelReady = false;
		m_LayerDescription.clear();
		m_LayerData.clear();
		m_Training = false;
		m_BatchSize = 1;
		m_TrainStep = 0;
		m_OptimizerType = OptimizerType::SGD;
		m_SGDParams = Math::SGDOptimzierParams();
		m_AdamParams = Math::AdamOptimizerParams();
		m_AdamStates.clear();
		m_AdamStatesBias.clear();
		m_AdamCorrection1 = 1.0f;
		m_AdamCorrection2 = 1.0f;
	}
}
