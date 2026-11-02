#pragma once

#include "AI/Tensor.h"

namespace AI::Math {

	void GPUOptmizerMathInit();

	struct SGDOptimzierParams {
		float learningRate = 0.01f;
	};

	void SGDUpdate(Tensor<float>& parameter, Tensor<float>& grad, SGDOptimzierParams& params);
	void GPUSGDUpdate(Tensor<float>& parameter, Tensor<float>& grad, SGDOptimzierParams& params);

	struct AdamOptimizerParams {
		float learningRate = 0.01f;
		float beta1 = 0.9f;
		float beta2 = 0.999f;
		float epsilon = 1e-8f;
		float weightDecay = 0.0f;
	};

	struct AdamOptimizerState {
		Tensor<float> m;
		Tensor<float> v;
	};

	void AdamWUpdate(Tensor<float>& param, Tensor<float>& grad, AdamOptimizerState& state, AdamOptimizerParams& params, float correction1, float correction2, bool decay);
	void GPUAdamWUpdate(Tensor<float>& param, Tensor<float>& grad, AdamOptimizerState& state, AdamOptimizerParams& params, float correction1, float correction2, bool decay);

}
