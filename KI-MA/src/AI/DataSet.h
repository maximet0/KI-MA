#pragma once

#include <filesystem>

#include "Tensor.h"

namespace AI {

	class DataSet {
	public:
		DataSet() = default;
		~DataSet() = default;

		void newDataSet(size_t dataCount, size_t inputSize, size_t resultSize, bool labeledResults = false);

		bool loadDataSet(std::filesystem::path datasetPath);
		void saveDataSet(std::filesystem::path datasetPath);

		bool loadDataSet(std::filesystem::path inputs, std::filesystem::path results);

		size_t getDataCount() const { return m_DataCount; }
		size_t getInputSize() const { return m_InputSize; }
		size_t getResultSize() const { return m_ResultSize; }

		Tensor<float> getInput(size_t index, size_t batchCount);
		Tensor<float> getResult(size_t index, size_t batchCount);

		Tensor<float>& getInputs() { return m_Inputs; }
		Tensor<float>& getResults() { return m_Results; }
	private:

		bool loadMNIST(std::ifstream& mnistInputs, std::ifstream& mnistResults);

		size_t m_DataCount = 0;
		
		size_t m_InputSize = 0;
		size_t m_ResultSize = 0;

		bool m_LabeledResults = false;

		Tensor<float> m_Inputs;
		Tensor<float> m_Results;
	};

}
