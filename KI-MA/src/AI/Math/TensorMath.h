#pragma once

#include "AI/Tensor.h"

namespace AI::Math {

	struct BroadcastPlan {
		bool continuous = false;
		bool valid = false;
		std::vector<size_t> resultShape;
		std::vector<std::vector<size_t>> effectiveStrides;

		bool matMulValid = false;
		bool matMulBatched = false;

		size_t M = 0;
		size_t N = 0;
		size_t K = 0;

		std::vector<size_t> matMulResultShape;
	};


	BroadcastPlan CreateBroadcastPlan(const std::vector<Tensor<float>*>& tensors);
	bool CanBroadcast(const std::vector<std::vector<size_t>>& shapes, std::vector<size_t>& resultShape);
	bool CanBroadcastMatMul(const std::vector<std::vector<size_t>>& shapes, std::vector<size_t>& resultShape);
	void GetEffectiveStrides(const std::vector<size_t>& shape, std::vector<size_t>& strides, const std::vector<size_t>& resultShape);
	
	void Add(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);
	void Sub(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);
	void Mul(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);
	void Div(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);

	void MulAdd(const Tensor<float>& a, const Tensor<float>& b, const Tensor<float>& c, Tensor<float>& result, const BroadcastPlan& plan);
	void MatMul(const Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);

	void MatMulNaive(const Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);

	void SiLU(const Tensor<float>& a, Tensor<float>& result);
	void SiLUGradient(const Tensor<float>& a, Tensor<float>& inputGradient, Tensor<float>& outputGradient);

	void ReLU(const Tensor<float>& a, Tensor<float>& result);
	void ReLUGradient(const Tensor<float>& a, Tensor<float>& inputGradient, Tensor<float>& outputGradient);

	void MSELoss(const Tensor<float>& results, const Tensor<float>& targets, float& loss);
	void MSELossGradient(const Tensor<float>& results, const Tensor<float>& targets, Tensor<float>& gradient);

	void Softmax(const Tensor<float>& input, Tensor<float>& output);

	void CELoss(const Tensor<float>& results, const Tensor<float>& targets, float& loss);
	void CELossGradient(const Tensor<float>& results, const Tensor<float>& targets, Tensor<float>& gradient);

	void Transpose(Tensor<float>& input, Tensor<float>& output);
	
	void Im2Col(const Tensor<float>& input, Tensor<float>& output, size_t kH, size_t kW, size_t sH, size_t sW, size_t pH, size_t pW);

}