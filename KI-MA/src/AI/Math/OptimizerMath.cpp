#include "OptimizerMath.h"

#include "TensorMath.h"
#include "AI/GPU/ComputePipeline.h"
#include "Core/Application.h"


namespace AI::Math {

	GPU::ComputePipeline g_SGDUpdatePipeline;
	GPU::ComputePipeline g_AdamWUpdatePipeline;

	extern Graphics::Renderer* g_Renderer;

	D3D12_ROOT_PARAMETER sgdUpdateParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
		.Constants = { 0, 0, 2 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER adamWUpdateParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
		.Constants = { 0, 0, 8 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER paramParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
		.Descriptor = { 0, 0 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER gradParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
		.Descriptor = { 0, 0 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER mParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
		.Descriptor = { 1, 0 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_ROOT_PARAMETER vParam = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
		.Descriptor = { 2, 0 },
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	void GPUOptmizerMathInit() {
		g_Renderer = Core::Application::getApplication()->getRenderer();

		g_SGDUpdatePipeline.setShaders("GPUTensorSGD.cso");
		g_SGDUpdatePipeline.addRootParameter(sgdUpdateParam);
		g_SGDUpdatePipeline.addRootParameter(paramParam);
		g_SGDUpdatePipeline.addRootParameter(gradParam);
		g_SGDUpdatePipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());

		g_AdamWUpdatePipeline.setShaders("GPUTensorAdamW.cso");
		g_AdamWUpdatePipeline.addRootParameter(adamWUpdateParam);
		g_AdamWUpdatePipeline.addRootParameter(paramParam);
		g_AdamWUpdatePipeline.addRootParameter(gradParam);
		g_AdamWUpdatePipeline.addRootParameter(mParam);
		g_AdamWUpdatePipeline.addRootParameter(vParam);
		g_AdamWUpdatePipeline.recreatePipelineState(Core::Application::getApplication()->getGraphicsContext()->getDevice().Get());
	}

	void SGDUpdate(Tensor<float>& param, Tensor<float>& grad, SGDOptimzierParams& params)
	{
		assert(param.getShape() == grad.getShape() && "Parameter and gradient tensors must have the same shape for SGD update.");

		if (param.isCPUDataDirty()) param.updateCPUData();
		if (grad.isCPUDataDirty()) grad.updateCPUData();
		param.clearCachedData();

		__m256 learningRateVec = _mm256_set1_ps(params.learningRate);
		size_t i = 0;
		for (; i + 8 <= grad.getTotalSize(); i+=8) {

			__m256 gradVec = _mm256_load_ps(&grad[i]);
			__m256 paramVec = _mm256_load_ps(&param[i]);
			paramVec = _mm256_sub_ps(paramVec, _mm256_mul_ps(learningRateVec, gradVec));
			_mm256_store_ps(&param[i], paramVec);
		}

		for (; i < grad.getTotalSize(); i++) {
			param[i] -= params.learningRate * grad[i];
		}
		param.markCPUWritten();
	}

	void GPUSGDUpdate(Tensor<float>& parameter, Tensor<float>& grad, SGDOptimzierParams& params) {
		assert(parameter.getShape() == grad.getShape() && "Parameter and gradient tensors must have the same shape for SGD update.");
		if (parameter.isGPUDataDirty()) parameter.updateGPUData();
		if (grad.isGPUDataDirty()) grad.updateGPUData();

		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();
		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		bufferManager.transitionState(parameter.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(grad.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		g_SGDUpdatePipeline.usePipeline(cmdList);

		uint32_t parameters[2] = {
			static_cast<uint32_t>(parameter.getTotalSize()),
			*reinterpret_cast<uint32_t*>(&params.learningRate)
		};

		cmdList->SetComputeRoot32BitConstants(0, 2, &parameters, 0);
		cmdList->SetComputeRootUnorderedAccessView(1, bufferManager.getBuffer(parameter.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(2, bufferManager.getBuffer(grad.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((static_cast<uint32_t>(parameter.getTotalSize()) + 255) / 256, 1, 1);
		parameter.markGPUWritten();
	}

	void AdamWUpdate(Tensor<float>& param, Tensor<float>& grad, AdamOptimizerState& state, AdamOptimizerParams& params, float correction1, float correction2, bool decay)
	{
		assert(param.getShape() == grad.getShape() && "Parameter and gradient tensors must have the same shape for AdamW update.");

		param.clearCachedData();

		if (param.isCPUDataDirty()) param.updateCPUData();
		if (grad.isCPUDataDirty()) grad.updateCPUData();
		if (state.m.isCPUDataDirty()) state.m.updateCPUData();
		if (state.v.isCPUDataDirty()) state.v.updateCPUData();

		float decayFactor = decay ? 1.0f - params.learningRate * params.weightDecay : 1.0f;

		__m256 beta1Vec = _mm256_set1_ps(params.beta1);
		__m256 beta2Vec = _mm256_set1_ps(params.beta2);
		__m256 oneMinusBeta1Vec = _mm256_set1_ps(1.0f - params.beta1);
		__m256 oneMinusBeta2Vec = _mm256_set1_ps(1.0f - params.beta2);
		__m256 learningRateVec = _mm256_set1_ps(params.learningRate);
		__m256 decayFactorVec = _mm256_set1_ps(decayFactor);
		__m256 epsilonVec = _mm256_set1_ps(params.epsilon);
		__m256 correction1Vec = _mm256_set1_ps(correction1);
		__m256 correction2Vec = _mm256_set1_ps(correction2);

		size_t i = 0;
		for (; i + 8 <= grad.getTotalSize(); i += 8) {
			__m256 gradVec = _mm256_load_ps(&grad[i]);
			__m256 mVec = _mm256_load_ps(&state.m[i]);
			__m256 vVec = _mm256_load_ps(&state.v[i]);
			__m256 oldParam = _mm256_load_ps(&param[i]);

			mVec = _mm256_add_ps(_mm256_mul_ps(beta1Vec, mVec), _mm256_mul_ps(oneMinusBeta1Vec, gradVec));
			vVec = _mm256_add_ps(_mm256_mul_ps(beta2Vec, vVec), _mm256_mul_ps(oneMinusBeta2Vec, _mm256_mul_ps(gradVec, gradVec)));

			__m256 mHatVec = _mm256_div_ps(mVec, correction1Vec);
			__m256 vHatVec = _mm256_div_ps(vVec, correction2Vec);

			__m256 denom = _mm256_add_ps(_mm256_sqrt_ps(vHatVec), epsilonVec);
			__m256 update = _mm256_div_ps(_mm256_mul_ps(learningRateVec, mHatVec), denom);


			_mm256_store_ps(&param[i], _mm256_sub_ps(_mm256_mul_ps(oldParam, decayFactorVec), update));
			_mm256_store_ps(&state.m[i], mVec);
			_mm256_store_ps(&state.v[i], vVec);

		}

		for (; i < grad.getTotalSize(); i++) {
			state.m[i] = params.beta1 * state.m[i] + (1.0f - params.beta1) * grad[i];
			state.v[i] = params.beta2 * state.v[i] + (1.0f - params.beta2) * grad[i] * grad[i];
			float mHat = state.m[i] / correction1;
			float vHat = state.v[i] / correction2;
			param[i] = param[i] * decayFactor - params.learningRate * mHat / (std::sqrt(vHat) + params.epsilon);
		}

		param.markCPUWritten();
		state.m.markCPUWritten();
		state.v.markCPUWritten();

	}

	void GPUAdamWUpdate(Tensor<float>& parameter, Tensor<float>& grad, AdamOptimizerState& state, AdamOptimizerParams& params, float correction1, float correction2, bool decay) {
		assert(parameter.getShape() == grad.getShape() && "Parameter and gradient tensors must have the same shape for AdamW update.");

		if (parameter.isGPUDataDirty()) parameter.updateGPUData();
		if (grad.isGPUDataDirty()) grad.updateGPUData();
		if (state.m.isGPUDataDirty()) state.m.updateGPUData();
		if (state.v.isGPUDataDirty()) state.v.updateGPUData();

		float decayFactor = decay ? 1.0f - params.learningRate * params.weightDecay : 1.0f;

		ID3D12GraphicsCommandList* cmdList = g_Renderer->getCmdList();
		Graphics::BufferManager& bufferManager = g_Renderer->getBufferManager();
		bufferManager.transitionState(parameter.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(grad.getGPUDataHandle(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		bufferManager.transitionState(state.m.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		bufferManager.transitionState(state.v.getGPUDataHandle(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		
		bufferManager.uavBarrier(state.m.getGPUDataHandle());
		bufferManager.uavBarrier(state.v.getGPUDataHandle());
		bufferManager.uavBarrier(parameter.getGPUDataHandle());

		g_AdamWUpdatePipeline.usePipeline(cmdList);

		uint32_t parameters[8] = {
			parameter.getTotalSize(),
			std::bit_cast<uint32_t>(params.learningRate), 
			std::bit_cast<uint32_t>(params.beta1), 
			std::bit_cast<uint32_t>(params.beta2), 
			std::bit_cast<uint32_t>(params.epsilon), 
			std::bit_cast<uint32_t>(decayFactor),
			std::bit_cast<uint32_t>(correction1), 
			std::bit_cast<uint32_t>(correction2), 
		};

		cmdList->SetComputeRoot32BitConstants(0, 8, parameters, 0);
		cmdList->SetComputeRootUnorderedAccessView(1, g_Renderer->getBufferManager().getBuffer(parameter.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootShaderResourceView(2, g_Renderer->getBufferManager().getBuffer(grad.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(3, g_Renderer->getBufferManager().getBuffer(state.m.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->SetComputeRootUnorderedAccessView(4, g_Renderer->getBufferManager().getBuffer(state.v.getGPUDataHandle())->GetGPUVirtualAddress());
		cmdList->Dispatch((static_cast<uint32_t>(parameter.getTotalSize()) + 255) / 256, 1, 1);
		parameter.markGPUWritten();
		state.m.markGPUWritten();
		state.v.markGPUWritten();
	}

}
