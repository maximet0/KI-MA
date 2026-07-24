#include "GraphicsPipeline.h"

#include <fstream>

#include "Core/Logger.h"

namespace Graphics {

	GraphicsPipeline::GraphicsPipeline()
	{

	}

	GraphicsPipeline::~GraphicsPipeline()
	{
		
	}

	void GraphicsPipeline::setShaders(const std::string& vertexShaderPath, const std::string& pixelShaderPath)
	{
		if (m_VertexShaderBytecode.pShaderBytecode != nullptr) {
			free((void*)m_VertexShaderBytecode.pShaderBytecode);
			m_VertexShaderBytecode.pShaderBytecode = nullptr;
		}

		if (m_PixelShaderBytecode.pShaderBytecode != nullptr) {
			free((void*)m_PixelShaderBytecode.pShaderBytecode);
			m_PixelShaderBytecode.pShaderBytecode = nullptr;
		}

		std::fstream vertexShaderFile(vertexShaderPath, std::ios::in | std::ios::binary);

		if (!vertexShaderFile.is_open()) {
			Core::Logger::Error("Failed to open {}", vertexShaderPath);
		}
		vertexShaderFile.seekg(0, std::ios::end);
		m_VertexShaderBytecode.BytecodeLength = vertexShaderFile.tellg();
		m_VertexShaderBytecode.pShaderBytecode = malloc(m_VertexShaderBytecode.BytecodeLength);
		vertexShaderFile.seekg(0, std::ios::beg);
		vertexShaderFile.read((char*)m_VertexShaderBytecode.pShaderBytecode, m_VertexShaderBytecode.BytecodeLength);
		vertexShaderFile.close();

		std::fstream pixelShaderFile(pixelShaderPath, std::ios::in | std::ios::binary);
		if (!pixelShaderFile.is_open()) {
			Core::Logger::Error("Failed to open {}", pixelShaderPath);
		}
		pixelShaderFile.seekg(0, std::ios::end);
		m_PixelShaderBytecode.BytecodeLength = pixelShaderFile.tellg();
		m_PixelShaderBytecode.pShaderBytecode = malloc(m_PixelShaderBytecode.BytecodeLength);
		pixelShaderFile.seekg(0, std::ios::beg);
		pixelShaderFile.read((char*)m_PixelShaderBytecode.pShaderBytecode, m_PixelShaderBytecode.BytecodeLength);
		pixelShaderFile.close();
	}

	void GraphicsPipeline::addInputElement(D3D12_INPUT_ELEMENT_DESC element)
	{
		m_InputLayout.push_back(element);
	}

	void GraphicsPipeline::addRootParameter(D3D12_ROOT_PARAMETER parameter)
	{
		m_RootParameters.push_back(parameter);
	}

	void GraphicsPipeline::addStaticSampler(D3D12_STATIC_SAMPLER_DESC sampler)
	{
		m_StaticSamplers.push_back(sampler);
	}

	void GraphicsPipeline::recreatePipelineState(ID3D12Device* device)
	{
		if (m_PipelineState != nullptr) {
			m_PipelineState.Reset();
		}

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = static_cast<uint32_t>(m_RootParameters.size());
		rootSignatureDesc.pParameters = m_RootParameters.data();
		rootSignatureDesc.NumStaticSamplers = static_cast<uint32_t>(m_StaticSamplers.size());
		rootSignatureDesc.pStaticSamplers = m_StaticSamplers.data();
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, nullptr);

		device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { m_InputLayout.data(), static_cast<uint32_t>(m_InputLayout.size()) };
		psoDesc.pRootSignature = m_RootSignature.Get();
		psoDesc.VS = m_VertexShaderBytecode;
		psoDesc.PS = m_PixelShaderBytecode;

		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
		psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;
		psoDesc.RasterizerState.MultisampleEnable = FALSE;
		psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;

		psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;

		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;


		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
	}

	void GraphicsPipeline::usePipeline(ID3D12GraphicsCommandList* cmdList)
	{
		if (m_PipelineState != nullptr && m_RootSignature != nullptr) {
			cmdList->SetPipelineState(m_PipelineState.Get());
			cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
		}
		else {
			Core::Logger::Error("Invalid PipelineState");
		}

	}

}
