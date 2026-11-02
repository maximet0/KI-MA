#include "DataSet.h"


#include <fstream>

namespace  AI {

	struct dtstHeader {
		char magic[4] = { '\n', '\n', '\n', '\n' };
		uint32_t dataCount;
		uint32_t inputSize;
		uint32_t resultSize;
		bool labeledResults;
	};

	void DataSet::newDataSet(size_t dataCount, size_t inputSize, size_t resultSize, bool labeledResults)
	{
		if (dataCount == 0 || inputSize == 0 || resultSize == 0) {
			Core::Logger::Error("Invalid dataset parameters.");
			return;
		}

		m_DataCount = dataCount;
		m_InputSize = inputSize;
		m_ResultSize = resultSize;
		m_LabeledResults = labeledResults;

		m_Inputs = Tensor<float>({ m_DataCount, m_InputSize });
		m_Results = Tensor<float>({ m_DataCount, m_ResultSize });

		if (m_LabeledResults)
			std::fill_n(m_Results.getDataPtr(), m_DataCount * m_ResultSize, 0.0f);
	}

	bool AI::DataSet::loadDataSet(std::filesystem::path datasetPath) {
		std::ifstream datasetFile(datasetPath, std::ios::binary);
		
		dtstHeader header;
		datasetFile.read(reinterpret_cast<char*>(&header), sizeof(header));
		if (header.magic[0] != 'D' || header.magic[1] != 'T' || header.magic[2] != 'S' || header.magic[3] != 'T') {
			Core::Logger::Error("Invalid dataset file format, or File doesn't exist.");
			return false;
		}

		m_DataCount = header.dataCount;
		m_InputSize = header.inputSize;
		m_ResultSize = header.resultSize;
		m_LabeledResults = header.labeledResults;

		m_Inputs = Tensor<float>({ m_DataCount, m_InputSize });
		m_Results = Tensor<float>({ m_DataCount, m_ResultSize });

		datasetFile.read(reinterpret_cast<char*>(m_Inputs.getDataPtr()), m_DataCount * m_InputSize * sizeof(float));
		if (m_LabeledResults) {
			std::vector<size_t> labels(m_DataCount);
			datasetFile.read(reinterpret_cast<char*>(labels.data()), m_DataCount * sizeof(size_t));
		
			std::fill_n(m_Results.getDataPtr(), m_DataCount * m_ResultSize, 0.0f);

			for (size_t i = 0; i < m_DataCount; ++i) {
				m_Results(i, labels[i]) = 1.0f;
			}
		}
		else {
			datasetFile.read(reinterpret_cast<char*>(m_Results.getDataPtr()), m_DataCount * m_ResultSize * sizeof(float));
		}

		datasetFile.close();

		return true;
	}

	void DataSet::saveDataSet(std::filesystem::path datasetPath) {
		std::ofstream datasetFile(datasetPath, std::ios::binary);

		dtstHeader header;
		header.magic[0] = 'D';
		header.magic[1] = 'T';
		header.magic[2] = 'S';
		header.magic[3] = 'T';
		header.dataCount = static_cast<uint32_t>(m_DataCount);
		header.inputSize = static_cast<uint32_t>(m_InputSize);
		header.resultSize = static_cast<uint32_t>(m_ResultSize);
		header.labeledResults = m_LabeledResults;

		datasetFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
		datasetFile.write(reinterpret_cast<const char*>(m_Inputs.getDataPtr()), m_DataCount * m_InputSize * sizeof(float));

		if (m_LabeledResults) {
			std::vector<size_t> labels(m_DataCount);

			for (size_t i = 0; i < m_DataCount; ++i) {
				labels[i] = std::distance(m_Results.getDataPtr() + i * m_ResultSize, std::max_element(m_Results.getDataPtr() + i * m_ResultSize, m_Results.getDataPtr() + (i + 1) * m_ResultSize));
			}
			datasetFile.write(reinterpret_cast<const char*>(labels.data()), m_DataCount * sizeof(size_t));
		}
		else datasetFile.write(reinterpret_cast<const char*>(m_Results.getDataPtr()), m_DataCount * m_ResultSize * sizeof(float));

		datasetFile.close();
	}

	uint32_t swapEndianess(uint32_t value) {
		return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) | ((value << 24) & 0xFF000000);
	}

	bool DataSet::loadDataSet(std::filesystem::path inputs, std::filesystem::path results) {

		bool result = false;

		if (!std::filesystem::exists(inputs) || !std::filesystem::exists(results)) {
			Core::Logger::Error("Input or Result file does not exist.");
			return false;
		}

		std::ifstream inputFile(inputs, std::ios::binary);
		std::ifstream resultFile(results, std::ios::binary);

		int32_t inputMagic = 0;
		int32_t resultMagic = 0;

		inputFile.read(reinterpret_cast<char*>(&inputMagic), sizeof(inputMagic));
		resultFile.read(reinterpret_cast<char*>(&resultMagic), sizeof(resultMagic));

		if (inputMagic == swapEndianess(0x00000803) && resultMagic == swapEndianess(0x00000801)) {
			result = loadMNIST(inputFile, resultFile);
		}
		else {
			Core::Logger::Error("Unsupported Import Format");
		}


		inputFile.close();
		resultFile.close();

		return result;	
	}

	Tensor<float> DataSet::getInput(size_t index, size_t batchCount) {
		return m_Inputs.view({ batchCount, m_InputSize }, index, 0).viewCopy();
	}

	Tensor<float> DataSet::getResult(size_t index, size_t batchCount) {
		return m_Results.view({ batchCount, m_ResultSize }, index, 0).viewCopy();
	}


	// MNIST Dataset.
	bool DataSet::loadMNIST(std::ifstream& mnistInputs, std::ifstream& mnistResults) {
		uint32_t inputCount = 0, resultCount = 0;
		uint32_t inputRows = 0;
		uint32_t inputCols = 0;

		mnistInputs.read(reinterpret_cast<char*>(&inputCount), sizeof(inputCount));
		mnistInputs.read(reinterpret_cast<char*>(&inputRows), sizeof(inputRows));
		mnistInputs.read(reinterpret_cast<char*>(&inputCols), sizeof(inputCols));

		mnistResults.read(reinterpret_cast<char*>(&resultCount), sizeof(resultCount));

		if (inputCount != resultCount) {
			Core::Logger::Error("MNIST Invalid");
			return false;
		}

		m_DataCount = swapEndianess(inputCount);
		m_InputSize = swapEndianess(inputRows) * swapEndianess(inputCols);
		m_ResultSize = 10; //MNIST has 10 classes (digits 0-9)

		std::vector<uint8_t> inputBuffer(m_DataCount * m_InputSize);

		mnistInputs.read(reinterpret_cast<char*>(inputBuffer.data()), inputBuffer.size());

		m_Inputs = Tensor<float>({ m_DataCount, m_InputSize });

		size_t i = 0;
		for (auto& val : inputBuffer) {
			m_Inputs[i++] = static_cast<float>(val) / 255.0f; // ubyte -> float
		}

		std::vector<uint8_t> labels(m_DataCount);
		mnistResults.read(reinterpret_cast<char*>(labels.data()), labels.size());
		
		m_Results = Tensor<float>({ m_DataCount, m_ResultSize });
		std::fill_n(m_Results.getDataPtr(), m_DataCount * m_ResultSize, 0.0f);

		i = 0;
		for (auto& label : labels) {
			m_Results(i++, label) = 1.0f;
		}

		return true;
	}


}

