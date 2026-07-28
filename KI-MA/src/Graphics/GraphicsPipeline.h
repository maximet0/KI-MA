#pragma once
#include <string>
#include <vector>

#include <d3d12.h>
#include <wrl.h>

namespace Graphics {
	struct PipelineSettings {
		D3D12_FILL_MODE fillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	};

	class GraphicsPipeline {
	public:
		GraphicsPipeline();
		~GraphicsPipeline();

		void setShaders(const std::string& vertexShaderPath, const std::string& pixelShaderPath);

		void addInputElement(D3D12_INPUT_ELEMENT_DESC element);
		void addRootParameter(D3D12_ROOT_PARAMETER parameter);
		void addStaticSampler(D3D12_STATIC_SAMPLER_DESC sampler);

		void resetInputElements() { m_InputLayout.clear(); }
		void resetRootParameters() { m_RootParameters.clear(); }
		void resetStaticSamplers() { m_StaticSamplers.clear(); }

		void recreatePipelineState(ID3D12Device* device, const PipelineSettings& settings);

		void usePipeline(ID3D12GraphicsCommandList* cmdList);
	private:

		std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;
		
		std::vector<D3D12_ROOT_PARAMETER> m_RootParameters;
		std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers;

		D3D12_SHADER_BYTECODE m_VertexShaderBytecode = {};
		D3D12_SHADER_BYTECODE m_PixelShaderBytecode = {};

		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	};
}