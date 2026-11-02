#pragma once
#include <string>
#include <vector>

#include <d3d12.h>
#include <wrl.h>

namespace AI::GPU {
	class ComputePipeline {
	public:
		ComputePipeline();
		~ComputePipeline();

		void setShaders(const std::string& computeShaderPath);

		void addRootParameter(D3D12_ROOT_PARAMETER parameter);
		void addStaticSampler(D3D12_STATIC_SAMPLER_DESC sampler);

		void resetRootParameters() { m_RootParameters.clear(); }
		void resetStaticSamplers() { m_StaticSamplers.clear(); }

		void recreatePipelineState(ID3D12Device* device);

		void usePipeline(ID3D12GraphicsCommandList* cmdList);
	private:

		std::vector<D3D12_ROOT_PARAMETER> m_RootParameters;
		std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers;

		D3D12_SHADER_BYTECODE m_ComputeShaderBytecode = {};

		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	};
}