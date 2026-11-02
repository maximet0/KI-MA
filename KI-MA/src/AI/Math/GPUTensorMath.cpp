#include "GPUTensorMath.h"

#include "AI/GPU/ComputePipeline.h"
#include "Core/Application.h"
#include "OptimizerMath.h"

namespace AI::Math {

	Graphics::Renderer* g_Renderer = nullptr;

	GPU::ComputePipeline g_AddSeqPipeline;
	GPU::ComputePipeline g_SubSeqPipeline;
	GPU::ComputePipeline g_MulSeqPipeline;
	GPU::ComputePipeline g_DivSeqPipeline;
	GPU::ComputePipeline g_MulAddPipeline;


	GPU::ComputePipeline g_AddScalPipeline;
	GPU::ComputePipeline g_SubScalPipeline;
	GPU::ComputePipeline g_MulScalPipeline;
	GPU::ComputePipeline g_DivScalPipeline;
	GPU::ComputePipeline g_MulAddScalPipeline;

	GPU::ComputePipeline g_AddBroadcastPipeline;
	GPU::ComputePipeline g_SubBroadcastPipeline;
	GPU::ComputePipeline g_MulBroadcastPipeline;
	GPU::ComputePipeline g_DivBroadcastPipeline;
	GPU::ComputePipeline g_MulAddBroadcastPipeline;

	GPU::ComputePipeline g_MatMulPipeline;

	GPU::ComputePipeline g_SiLUPipeline;
	GPU::ComputePipeline g_ReLUPipeline;

	GPU::ComputePipeline g_SiLUGradPipeline;
	GPU::ComputePipeline g_ReLUGradPipeline;

	GPU::ComputePipeline g_MatMulNaivePipeline;

	GPU::ComputePipeline g_SoftmaxPipeline;

	GPU::ComputePipeline g_TransposePipeline;

	GPU::ComputePipeline g_CELossGradPipeline;

	GPU::ComputePipeline g_BiasGradientPipeline;

	D3D12_ROOT_PARAMETER elemCountParam = { 
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
		.Constants = { 0, 0, 1 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL 
	};

	D3D12_ROOT_PARAMETER broadcast2ValParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
		.Constants = { 0, 0, 16 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER broadcast3ValParam = {
	.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
	.Constants = { 0, 0, 20 },
	.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};


	D3D12_ROOT_PARAMETER matMulParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
		.Constants = { 0, 0, 3 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER twoRootConstParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
		.Constants = { 0, 0, 2 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER aParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
		.Descriptor = { 0, 0 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER bParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
		.Descriptor = { 1, 0 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER cParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
		.Descriptor = { 2, 0 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER resultParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
		.Descriptor = { 0, 0 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	void setup1ValPipeline(GPU::ComputePipeline& pipeline, const std::string& shaderPath) {
		pipeline.setShaders(shaderPath);
		pipeline.addRootParameter(elemCountParam);
		pipeline.addRootParameter(aParam);
		pipeline.addRootParameter(resultParam);
		pipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());
	}

	void setup2ValPipeline(GPU::ComputePipeline& pipeline, const std::string& shaderPath) {
		pipeline.setShaders(shaderPath);
		pipeline.addRootParameter(elemCountParam);
		pipeline.addRootParameter(aParam);
		pipeline.addRootParameter(bParam);
		pipeline.addRootParameter(resultParam);
		pipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());
	}

	void setup3ValPipeline(GPU::ComputePipeline& pipeline, const std::string& shaderPath) {
		pipeline.setShaders(shaderPath);
		pipeline.addRootParameter(elemCountParam);
		pipeline.addRootParameter(aParam);
		pipeline.addRootParameter(bParam);
		pipeline.addRootParameter(cParam);
		pipeline.addRootParameter(resultParam);
		pipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());
	}

	void setup2ValBroadcastPipeline(GPU::ComputePipeline& pipeline, const std::string& shaderPath) {
		pipeline.setShaders(shaderPath);
		pipeline.addRootParameter(broadcast2ValParam);
		pipeline.addRootParameter(aParam);
		pipeline.addRootParameter(bParam);
		pipeline.addRootParameter(resultParam);
		pipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());
	}

	void setup3ValBroadcastPipeline(GPU::ComputePipeline& pipeline, const std::string& shaderPath) {
		pipeline.setShaders(shaderPath);
		pipeline.addRootParameter(broadcast3ValParam);
		pipeline.addRootParameter(aParam);
		pipeline.addRootParameter(bParam);
		pipeline.addRootParameter(cParam);
		pipeline.addRootParameter(resultParam);
		pipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());
	}

	void setupMatMulPipeline(GPU::ComputePipeline& pipeline, const std::string& shaderPath) {
		pipeline.setShaders(shaderPath);
		pipeline.addRootParameter(matMulParam);
		pipeline.addRootParameter(aParam);
		pipeline.addRootParameter(bParam);
		pipeline.addRootParameter(resultParam);
		pipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());
	}

	void setupSoftmaxPipeline(GPU::ComputePipeline& pipeline, const std::string& shaderPath) {
		pipeline.setShaders(shaderPath);
		pipeline.addRootParameter(twoRootConstParam);
		pipeline.addRootParameter(aParam);
		pipeline.addRootParameter(resultParam);
		pipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());
	}

	void setupCELossGradPipeline(GPU::ComputePipeline& pipeline, const std::string& shaderPath) {
		pipeline.setShaders(shaderPath);
		pipeline.addRootParameter(twoRootConstParam);
		pipeline.addRootParameter(aParam);
		pipeline.addRootParameter(bParam);
		pipeline.addRootParameter(resultParam);
		pipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());
	}

	void GPUInit() {
		g_Renderer = Core::Application::getApplication()->getRenderer();

		GPUOptmizerMathInit();

		setup2ValPipeline(g_AddSeqPipeline, "GPUTensorAddSeq.cso");
		setup2ValPipeline(g_SubSeqPipeline, "GPUTensorSubSeq.cso");
		setup2ValPipeline(g_MulSeqPipeline, "GPUTensorMulSeq.cso");
		setup2ValPipeline(g_DivSeqPipeline, "GPUTensorDivSeq.cso");
		setup3ValPipeline(g_MulAddPipeline, "GPUTensorMulAddSeq.cso");

		setup2ValPipeline(g_AddScalPipeline, "GPUTensorAddScal.cso");
		setup2ValPipeline(g_SubScalPipeline, "GPUTensorSubScal.cso");
		setup2ValPipeline(g_MulScalPipeline, "GPUTensorMulScal.cso");
		setup2ValPipeline(g_DivScalPipeline, "GPUTensorDivScal.cso");
		setup3ValPipeline(g_MulAddScalPipeline, "GPUTensorMulAddScal.cso");

		setup2ValBroadcastPipeline(g_AddBroadcastPipeline, "GPUTensorAddBroadcast.cso");
		setup2ValBroadcastPipeline(g_SubBroadcastPipeline, "GPUTensorSubBroadcast.cso");
		setup2ValBroadcastPipeline(g_MulBroadcastPipeline, "GPUTensorMulBroadcast.cso");
		setup2ValBroadcastPipeline(g_DivBroadcastPipeline, "GPUTensorDivBroadcast.cso");
		setup3ValBroadcastPipeline(g_MulAddBroadcastPipeline, "GPUTensorMulAddBroadcast.cso");

		setupMatMulPipeline(g_MatMulPipeline, "GPUTensorMatMul.cso");
		setupMatMulPipeline(g_MatMulNaivePipeline, "GPUTensorMatMulNaive.cso");

		setup1ValPipeline(g_SiLUPipeline, "GPUTensorSiLU.cso");
		setup1ValPipeline(g_ReLUPipeline, "GPUTensorReLU.cso");

		setup1ValPipeline(g_SiLUGradPipeline, "GPUTensorSiLUGrad.cso");
		setup1ValPipeline(g_ReLUGradPipeline, "GPUTensorReLUGrad.cso");

		setupSoftmaxPipeline(g_SoftmaxPipeline, "GPUTensorSoftmax.cso");

		setupSoftmaxPipeline(g_TransposePipeline, "GPUTensorTranspose.cso");

		setupCELossGradPipeline(g_CELossGradPipeline, "GPUTensorCELossGrad.cso");
		setupSoftmaxPipeline(g_BiasGradientPipeline, "GPUTensorGenBiasGrad.cso");
	}

	void beginGPUMath() {
		g_Renderer->beginCmdList(Core::Application::getApplication()->getSwapchain()->getBackBufferIndex());
	}

	void endGPUMath()
	{
		g_Renderer->endCmdList();
	}

	void dispatch2ValBroadcast(GPU::ComputePipeline& pipeline, Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		bufferManager.transitionState(a.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(b.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(result.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pipeline.usePipeline(cmdList);
		uint32_t totalSize = static_cast<uint32_t>(result.getTotalSize());
		uint32_t dimensions = static_cast<uint32_t>(plan.resultShape.size() - 1);

		uint32_t outputShape[4] = { 1, 1, 1, 1 };
		uint32_t stridesA[4] = { 0, 0, 0, 0 };
		uint32_t stridesB[4] = { 0, 0, 0, 0 };

		for (size_t i = 0; i < plan.resultShape.size(); i++) {
			outputShape[3 - i] = static_cast<uint32_t>(plan.resultShape[dimensions - i]);
			stridesA[3 - i] = static_cast<uint32_t>(plan.effectiveStrides[0][dimensions - i]);
			stridesB[3 - i] = static_cast<uint32_t>(plan.effectiveStrides[1][dimensions - i]);

		}
			
		uint32_t rootConstants[16] = { totalSize, 0, 0, 0,
			outputShape[0], outputShape[1], outputShape[2], outputShape[3],
			stridesA[0], stridesA[1], stridesA[2], stridesA[3],
			stridesB[0], stridesB[1], stridesB[2], stridesB[3] };

		cmdList->SetComputeRoot32BitConstants(0, 16, rootConstants, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(a.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(2, bufferManager.getBuffer(b.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(3, bufferManager.getBuffer(result.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((totalSize + 255) / 256, 1, 1);
		result.markGPUWritten();
	}

	void dispatch3ValBroadcast(GPU::ComputePipeline& pipeline, Tensor<float>& a, Tensor<float>& b, Tensor<float>& c, Tensor<float>& result, const BroadcastPlan& plan) {
		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		bufferManager.transitionState(a.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(b.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(c.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(result.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pipeline.usePipeline(cmdList);
		uint32_t totalSize = static_cast<uint32_t>(result.getTotalSize());
		uint32_t dimensions = static_cast<uint32_t>(plan.resultShape.size() - 1);

		if (plan.resultShape.size() > 4) {
			Core::Logger::Error("GPUADD, More than 4 Dimensions not supported");
			return;
		}

		uint32_t outputShape[4] = { 1, 1, 1, 1 };
		uint32_t stridesA[4] = { 0, 0, 0, 0 };
		uint32_t stridesB[4] = { 0, 0, 0, 0 };
		uint32_t stridesC[4] = { 0, 0, 0, 0 };

		for (size_t i = 0; i < plan.resultShape.size(); i++) {
			outputShape[3 - i] = static_cast<uint32_t>(plan.resultShape[dimensions - i]);
			stridesA[3 - i] = static_cast<uint32_t>(plan.effectiveStrides[0][dimensions - i]);
			stridesB[3 - i] = static_cast<uint32_t>(plan.effectiveStrides[1][dimensions - i]);
			stridesC[3 - i] = static_cast<uint32_t>(plan.effectiveStrides[2][dimensions - i]);
		}
			
		uint32_t rootConstants[20] = { totalSize, 0, 0, 0,
			outputShape[0], outputShape[1], outputShape[2], outputShape[3],
			stridesA[0], stridesA[1], stridesA[2], stridesA[3],
			stridesB[0], stridesB[1], stridesB[2], stridesB[3], 
			stridesC[0], stridesC[1], stridesC[2], stridesC[3] };

		cmdList->SetComputeRoot32BitConstants(0, 20, rootConstants, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(a.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(2, bufferManager.getBuffer(b.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(3, bufferManager.getBuffer(c.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(4, bufferManager.getBuffer(result.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((totalSize + 255) / 256, 1, 1);
		result.markGPUWritten();
	}

	void dispatch2Val(GPU::ComputePipeline& pipeline, Tensor<float>& a, Tensor<float>& b, Tensor<float>& result) {
		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		bufferManager.transitionState(a.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(b.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(result.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pipeline.usePipeline(cmdList);
		uint32_t totalSize = static_cast<uint32_t>(result.getTotalSize());
		cmdList->SetComputeRoot32BitConstants(0, 1, &totalSize, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(a.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(2, bufferManager.getBuffer(b.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(3, bufferManager.getBuffer(result.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((totalSize + 255) / 256, 1, 1);
		result.markGPUWritten();
	}

	void dispatch3Val(GPU::ComputePipeline& pipeline, Tensor<float>& a, Tensor<float>& b, Tensor<float>& c, Tensor<float>& result) {
		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		bufferManager.transitionState(a.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(b.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(c.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(result.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pipeline.usePipeline(cmdList);
		uint32_t totalSize = static_cast<uint32_t>(result.getTotalSize());
		cmdList->SetComputeRoot32BitConstants(0, 1, &totalSize, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(a.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(2, bufferManager.getBuffer(b.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(3, bufferManager.getBuffer(c.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(4, bufferManager.getBuffer(result.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((totalSize + 255) / 256, 1, 1);
		result.markGPUWritten();
	}

	void GPUAdd(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid for GPUAdd.");

		if (a.isGPUDataDirty()) a.updateGPUData();
		if (b.isGPUDataDirty()) b.updateGPUData();
		if (!result.gpuDataExists()) result.createGPUData();

		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();

		if (plan.continuous) {
			dispatch2Val(g_AddSeqPipeline, a, b, result);
			return;
		}

		if (b.getTotalSize() == 1) {
			dispatch2Val(g_AddScalPipeline, a, b, result);
			return;
		}

		dispatch2ValBroadcast(g_AddBroadcastPipeline, a, b, result, plan);
	}

	void GPUSub(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid for GPUAdd.");
		if (a.isGPUDataDirty()) a.updateGPUData();
		if (b.isGPUDataDirty()) b.updateGPUData();
		if (!result.gpuDataExists()) result.createGPUData();

		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();

		if (plan.continuous) {
			dispatch2Val(g_SubSeqPipeline, a, b, result);
			return;
		}

		if (b.getTotalSize() == 1) {
			dispatch2Val(g_SubScalPipeline, a, b, result);
			return;
		}
		
		dispatch2ValBroadcast(g_SubBroadcastPipeline, a, b, result, plan);

	}

	void GPUMul(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid for GPUAdd.");
		if (a.isGPUDataDirty()) a.updateGPUData();
		if (b.isGPUDataDirty()) b.updateGPUData();
		if (!result.gpuDataExists()) result.createGPUData();

		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();

		if (plan.continuous) {
			dispatch2Val(g_MulSeqPipeline, a, b, result);
			return;
		}

		if (b.getTotalSize() == 1) {
			dispatch2Val(g_MulScalPipeline, a, b, result);
			return;
		}

		dispatch2ValBroadcast(g_MulBroadcastPipeline, a, b, result, plan);
	}

	void GPUDiv(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid for GPUAdd.");
		if (a.isGPUDataDirty()) a.updateGPUData();
		if (b.isGPUDataDirty()) b.updateGPUData();
		if (!result.gpuDataExists()) result.createGPUData();

		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();

		if (plan.continuous) {
			dispatch2Val(g_DivSeqPipeline, a, b, result);
			return;
		}

		if (b.getTotalSize() == 1) {
			dispatch2Val(g_DivScalPipeline, a, b, result);
			return;
		}

		dispatch2ValBroadcast(g_DivBroadcastPipeline, a, b, result, plan);
	}

	void GPUMulAdd(Tensor<float>& a, Tensor<float>& b, Tensor<float>& c, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid for GPUAdd.");
		if (a.isGPUDataDirty()) a.updateGPUData();
		if (b.isGPUDataDirty()) b.updateGPUData();
		if (c.isGPUDataDirty()) c.updateGPUData();
		if (!result.gpuDataExists()) result.createGPUData();

		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();

		if (plan.continuous) {
			dispatch3Val(g_MulAddPipeline, a, b, c, result);
			return;
		}

		if (b.getTotalSize() == 1 && c.getTotalSize() == 1) {
			dispatch3Val(g_MulAddScalPipeline, a, b, c, result);
			return;
		}

		dispatch3ValBroadcast(g_MulAddBroadcastPipeline, a, b, c, result, plan);
	}


#define OUT_TILE_ROWS 64
#define OUT_TILE_COLS 64

	void GPUMatMul(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.matMulValid && "Broadcast plan is not valid for GPUMatMul.");
		assert(result.getShape() == plan.matMulResultShape && "Result tensor shape must match planned shape.");

		if (a.isGPUDataDirty()) a.updateGPUData();
		if (b.isGPUDataDirty()) b.updateGPUData();
		if (!result.gpuDataExists()) result.createGPUData();

		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();
		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();

		bufferManager.transitionState(result.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(a.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(b.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		g_MatMulPipeline.usePipeline(cmdList);

		if (plan.matMulValid) {
			uint32_t params[3];
			params[0] = static_cast<uint32_t>(plan.M);
			params[1] = static_cast<uint32_t>(plan.N);
			params[2] = static_cast<uint32_t>(plan.K);
			cmdList->SetComputeRoot32BitConstants(0, 3, &params, 0);
			cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(a.getGPUDataHandle())->GetGPUVirtualAddress());
			cmdList->SetComputeRootShaderResourceView(2, bufferManager.getBuffer(b.getGPUDataHandle())->GetGPUVirtualAddress());
			cmdList->SetComputeRootUnorderedAccessView(3, bufferManager.getBuffer(result.getGPUDataHandle())->GetGPUVirtualAddress());
			cmdList->Dispatch((params[1] + OUT_TILE_COLS - 1) / OUT_TILE_COLS, (params[0] + OUT_TILE_ROWS - 1) / OUT_TILE_ROWS, 1);
			result.markGPUWritten();
		}
	}

	void GPUMatMulNaive(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan)
	{
		assert(result.getShape() == plan.matMulResultShape && "Result tensor shape must match planned shape.");

		if (a.isGPUDataDirty()) a.updateGPUData();
		if (b.isGPUDataDirty()) b.updateGPUData();
		if (!result.gpuDataExists()) result.createGPUData();

		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();

		bufferManager.transitionState(result.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(a.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(b.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		g_MatMulNaivePipeline.usePipeline(cmdList);

		if (plan.matMulValid) {
			uint32_t params[3];
			params[0] = static_cast<uint32_t>(plan.M);
			params[1] = static_cast<uint32_t>(plan.N);
			params[2] = static_cast<uint32_t>(plan.K);
			cmdList->SetComputeRoot32BitConstants(0, 3, &params, 0);
			cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(a.getGPUDataHandle())->GetGPUVirtualAddress());
			cmdList->SetComputeRootShaderResourceView(2, bufferManager.getBuffer(b.getGPUDataHandle())->GetGPUVirtualAddress());
			cmdList->SetComputeRootUnorderedAccessView(3, bufferManager.getBuffer(result.getGPUDataHandle())->GetGPUVirtualAddress());
			cmdList->Dispatch((params[0] * params[1] + 256 - 1) / 256, 1, 1);
			result.markGPUWritten();
		}
	}

	void dispatch1Val(GPU::ComputePipeline& pipeline, Tensor<float>& a, Tensor<float>& result) {
		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		bufferManager.transitionState(result.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(a.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		pipeline.usePipeline(cmdList);
		uint32_t totalSize = static_cast<uint32_t>(result.getTotalSize());
		cmdList->SetComputeRoot32BitConstants(0, 1, &totalSize, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(a.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(2, bufferManager.getBuffer(result.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((totalSize + 255) / 256, 1, 1);
		result.markGPUWritten();
	}

	void GPUSiLU(Tensor<float>& a, Tensor<float>& result) {
		assert(a.getTotalSize() == result.getTotalSize());

		if (a.isGPUDataDirty()) a.updateGPUData();
		if (!result.gpuDataExists()) result.createGPUData();

		dispatch1Val(g_SiLUPipeline, a, result);
	}

	void GPUSiLUGradient(Tensor<float>& a, Tensor<float>& outputGradient, Tensor<float>& inputGradient) {
		assert(outputGradient.getDataPtr() == inputGradient.getDataPtr());
		assert(a.getTotalSize() == outputGradient.getTotalSize());

		if (a.isGPUDataDirty()) a.updateGPUData();
		if (outputGradient.isGPUDataDirty()) outputGradient.updateGPUData();

		dispatch1Val(g_SiLUGradPipeline, a, inputGradient);
	}

	void GPUReLU(Tensor<float>& a, Tensor<float>& result) {
		assert(a.getTotalSize() == result.getTotalSize());

		if (a.isGPUDataDirty()) a.updateGPUData();
		if (!result.gpuDataExists()) result.createGPUData();

		dispatch1Val(g_ReLUPipeline, a, result);
	}

	void GPUReLUGradient(Tensor<float>& a, Tensor<float>& outputGradient, Tensor<float>& inputGradient)
	{
		assert(a.getTotalSize() == inputGradient.getTotalSize());
		assert(outputGradient.getDataPtr() == inputGradient.getDataPtr());

		if (a.isGPUDataDirty()) a.updateGPUData();
		if (inputGradient.isGPUDataDirty()) inputGradient.updateGPUData();

		dispatch1Val(g_ReLUGradPipeline, a, inputGradient);
	}

	void GPUMSELoss(Tensor<float>&results, Tensor<float>&targets, float& loss)
	{}

	void GPUMSELossGradient(Tensor<float>&results, Tensor<float>&targets, Tensor<float>&gradient){
		assert(results.getTotalSize() == targets.getTotalSize());
		assert(results.getTotalSize() == gradient.getTotalSize());

		if (results.isGPUDataDirty()) results.updateGPUData();
		if (targets.isGPUDataDirty()) targets.updateGPUData();
		if (!gradient.gpuDataExists()) gradient.createGPUData();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		bufferManager.transitionState(gradient.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(results.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(targets.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		g_CELossGradPipeline.usePipeline(cmdList);
		uint32_t params[2];
		params[0] = static_cast<uint32_t>(results.getTotalSize());
		params[1] = static_cast<uint32_t>(results.getTotalSize());
		cmdList->SetComputeRoot32BitConstants(0, 2, &params, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(results.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(2, bufferManager.getBuffer(targets.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(3, bufferManager.getBuffer(gradient.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((params[0] + 255) / 256, 1, 1);

		gradient.markGPUWritten();
	}

	void GPUSoftmax(Tensor<float>&input, Tensor<float>&output){

		if (input.isGPUDataDirty()) input.updateGPUData();
		if (!output.gpuDataExists()) output.createGPUData();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		bufferManager.transitionState(output.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(input.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		g_SoftmaxPipeline.usePipeline(cmdList);
		uint32_t params[2];
		params[0] = static_cast<uint32_t>(input.getShape()[0]);
		params[1] = static_cast<uint32_t>(input.getShape()[1]);
		cmdList->SetComputeRoot32BitConstants(0, 2, &params, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(input.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(2, bufferManager.getBuffer(output.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((params[0] + 255) / 256, 1, 1);
		output.markGPUWritten();
	}

	void GPUCELoss(Tensor<float>&results, Tensor<float>&targets, float& loss)
	{}

	void GPUCELossGradient(Tensor<float>& results, Tensor<float>& targets, Tensor<float>& gradient) {
		assert(results.getTotalSize() == targets.getTotalSize());
		assert(results.getTotalSize() == gradient.getTotalSize());

		if (results.isGPUDataDirty()) results.updateGPUData();
		if (targets.isGPUDataDirty()) targets.updateGPUData();
		if (!gradient.gpuDataExists()) gradient.createGPUData();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		bufferManager.transitionState(gradient.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(results.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(targets.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		g_CELossGradPipeline.usePipeline(cmdList);
		uint32_t params[2];
		params[0] = static_cast<uint32_t>(results.getTotalSize());
		params[1] = static_cast<uint32_t>(results.getShape()[0]);
		cmdList->SetComputeRoot32BitConstants(0, 2, &params, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(results.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(2, bufferManager.getBuffer(targets.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(3, bufferManager.getBuffer(gradient.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((params[0] + 255) / 256, 1, 1);
		gradient.markGPUWritten();
	}


	void GPUTranspose(Tensor<float>&input, Tensor<float>&output) {
		assert(input.getShape().size() == 2 && output.getShape().size() == 2 && "Only 2D tensors are supported for transpose.");
		assert(input.getShape()[0] == output.getShape()[1] && input.getShape()[1] == output.getShape()[0] && "Output tensor shape must be the transpose of the input tensor shape.");

		if (input.isGPUDataDirty()) input.updateGPUData();
		if (!output.gpuDataExists()) output.createGPUData();

		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		bufferManager.transitionState(output.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(input.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		g_TransposePipeline.usePipeline(cmdList);
		uint32_t params[2];
		params[0] = static_cast<uint32_t>(output.getShape()[0]);
		params[1] = static_cast<uint32_t>(output.getShape()[1]);
		cmdList->SetComputeRoot32BitConstants(0, 2, &params, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(input.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(2, bufferManager.getBuffer(output.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((params[1] + 15) / 16, (params[0] + 15) / 16, 1);
		output.markGPUWritten();
	}
	
	void GPUBiasGradient(Tensor<float>& gradient, Tensor<float>& biasGradient) {
		assert(gradient.getShape().size() == 2);
		assert(biasGradient.getShape().size() == 2);
		assert(biasGradient.getShape()[0] == 1);
		assert(gradient.getShape()[1] == biasGradient.getShape()[1]);

		if (gradient.isGPUDataDirty()) gradient.updateGPUData();
		if (!biasGradient.gpuDataExists()) biasGradient.createGPUData();

		auto& bufferManager = g_Renderer->getBufferManager();
		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();

		g_BiasGradientPipeline.usePipeline(cmdList);

		uint32_t parameters[2] = {
			static_cast<uint32_t>(gradient.getShape()[0]),
			static_cast<uint32_t>(gradient.getShape()[1])
		};

		bufferManager.transitionState(gradient.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(biasGradient.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		cmdList->SetComputeRoot32BitConstants(0, 2, parameters, 0);
		cmdList->SetComputeRootShaderResourceView(1, bufferManager.getBuffer(gradient.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(2, bufferManager.getBuffer(biasGradient.getGPUDataHandle())->GetGPUVirtualAddress());

		cmdList->Dispatch((parameters[1] + 255) / 256, 1, 1);

		biasGradient.markGPUWritten();
	}
}

