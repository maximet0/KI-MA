#pragma once

#include "Tensor.h"

namespace AI::Math {

	struct BroadcastPlan {
		bool continious = false;
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

	template<typename T>
	BroadcastPlan CreateBroadcastPlan(const std::vector<TensorView<T>>& tensors);
	bool CanBroadcast(const std::vector<std::vector<size_t>>& shapes, std::vector<size_t>& resultShape);
	bool CanBroadcastMatMul(const std::vector<std::vector<size_t>>& shapes, std::vector<size_t>& resultShape);
	void GetEffectiveStrides(const std::vector<size_t>& shape, std::vector<size_t>& strides, const std::vector<size_t>& resultShape);
	
	template<typename T>
	void Add(const TensorView<T>& a, const TensorView<T>& b, TensorView<T>& result, const BroadcastPlan& plan);
	template <typename T>
	void Mul(const TensorView<T>& a, const TensorView<T>& b, TensorView<T>& result, const BroadcastPlan& plan);
	template <typename T>
	void MulAdd(const TensorView<T>& a, const TensorView<T>& b, const TensorView<T>& c, TensorView<T>& result, const BroadcastPlan& plan);

	void MatMul(const Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan);

	template <typename T>
	void SiLU(const Tensor<T>& a, Tensor<T>& result);
}