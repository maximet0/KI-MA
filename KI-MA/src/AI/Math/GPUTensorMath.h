#pragma once

#include "AI/Tensor.h"
#include "TensorMath.h"

namespace AI::Math {
	void GPUInit();

	void beginGPUMath();
	void endGPUMath();

	void GPUAdd(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);
	void GPUSub(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);
	void GPUMul(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);
	void GPUDiv(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);
	void GPUMulAdd(Tensor<float>& a, Tensor<float>& b, Tensor<float>& c, Tensor<float>& result, const BroadcastPlan& plan);
	void GPUMatMul(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);

	void GPUMatMulNaive(Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);

	void GPUSiLU(Tensor<float>& a, Tensor<float>& result);
	void GPUSiLUGradient(Tensor<float>& a, Tensor<float>& inputGradient, Tensor<float>& outputGradient);
	void GPUReLU(Tensor<float>& a, Tensor<float>& result);
	void GPUReLUGradient(Tensor<float>& a, Tensor<float>& inputGradient, Tensor<float>& outputGradient);
	void GPUMSELoss(Tensor<float>& results, Tensor<float>& targets, float& loss);
	void GPUMSELossGradient(Tensor<float>& results, Tensor<float>& targets, Tensor<float>& gradient);
	void GPUSoftmax(Tensor<float>& input, Tensor<float>& output);
	void GPUCELoss(Tensor<float>& results, Tensor<float>& targets, float& loss);
	void GPUCELossGradient(Tensor<float>& results, Tensor<float>& targets, Tensor<float>& gradient);
	void GPUTranspose(Tensor<float>& input, Tensor<float>& output);

	void GPUBiasGradient(Tensor<float>& gradient, Tensor<float>& biasGradient);

}
