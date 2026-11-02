#include "ComputePipeline.h"

#include <fstream>

#include "Core/Logger.h"


namespace AI::GPU {
	ComputePipeline::ComputePipeline() {
		
	}
	
	ComputePipeline::~ComputePipeline() {

	}

	void ComputePipeline::setShaders(const std::string& computeShaderPath) {
		if (m_ComputeShaderBytecode.pShaderBytecode != nullptr) {
			free((void*)m_ComputeShaderBytecode.pShaderBytecode);
			m_ComputeShaderBytecode.pShaderBytecode = nullptr;
		}

		std::fstream computeShaderFile(computeShaderPath, std::ios::in | std::ios::binary);

		if (!computeShaderFile.is_open()) {
			Core::Logger::Error("Failed to open {}", computeShaderPath);
		}

		computeShaderFile.seekg(0, std::ios::end);
		m_ComputeShaderBytecode.BytecodeLength = computeShaderFile.tellg();
		m_ComputeShaderBytecode.pShaderBytecode = malloc(m_ComputeShaderBytecode.BytecodeLength);
		computeShaderFile.seekg(0, std::ios::beg);
		computeShaderFile.read((char*)m_ComputeShaderBytecode.pShaderBytecode, m_ComputeShaderBytecode.BytecodeLength);
		computeShaderFile.close();
	}
	
	void ComputePipeline::addRootParameter(D3D12_ROOT_PARAMETER parameter) {
		m_RootParameters.push_back(parameter);
	}
	
	void ComputePipeline::addStaticSampler(D3D12_STATIC_SAMPLER_DESC sampler) {
		m_StaticSamplers.push_back(sampler);
	}
	
	void ComputePipeline::recreatePipelineState(ID3D12Device * device) {
		if (m_PipelineState != nullptr) {
			m_PipelineState.Reset();
		}

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = static_cast<uint32_t>(m_RootParameters.size());
		rootSignatureDesc.pParameters = m_RootParameters.data();
		rootSignatureDesc.NumStaticSamplers = static_cast<uint32_t>(m_StaticSamplers.size());
		rootSignatureDesc.pStaticSamplers = m_StaticSamplers.data();
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, nullptr);

		device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_RootSignature.Get();
		psoDesc.CS = m_ComputeShaderBytecode;
		
		device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
	}
	
	void ComputePipeline::usePipeline(ID3D12GraphicsCommandList * cmdList) {
		if (m_PipelineState != nullptr && m_RootSignature != nullptr) {
			cmdList->SetPipelineState(m_PipelineState.Get());
			cmdList->SetComputeRootSignature(m_RootSignature.Get());
		}
		else {
			Core::Logger::Error("Invalid PipelineState");
		}

	}
}


