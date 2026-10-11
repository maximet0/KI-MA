#include "TensorMath.h"

#include <assert.h>

#include <immintrin.h>
#include <functional>
#include <algorithm>
#include <execution>
#include <ranges>

namespace AI::Math {
	template<typename T>
	BroadcastPlan CreateBroadcastPlan(const std::vector<TensorView<T>>& tensors)
	{
		BroadcastPlan plan;

		std::vector<std::vector<size_t>> shapes;
		shapes.reserve(tensors.size());
		plan.effectiveStrides.reserve(tensors.size());

		for (const auto& tensor : tensors) {
			shapes.push_back(tensor.getShape());
			plan.effectiveStrides.push_back(tensor.getStrides());
		}

		if (shapes.size() == 1 || std::all_of(shapes.begin(), shapes.end(), [&](const std::vector<size_t>& shape) { return shape == shapes[0]; })) {
			plan.continious = true;
			plan.valid = true;
			plan.resultShape = shapes[0];

		}
		else {
			plan.valid = CanBroadcast(shapes, plan.resultShape);
			if (plan.valid) {
				for (size_t i = 0; i < tensors.size(); i++) {
					GetEffectiveStrides(shapes[i], plan.effectiveStrides[i], plan.resultShape);
				}
			}
		}
		
		if(shapes.size() == 2 && std::all_of(shapes.begin(), shapes.end(), [](const std::vector<size_t>& shape) { return shape.size() == 2; }) && shapes[0][1] == shapes[1][0]) {
			const size_t M = shapes[0][0];
			const size_t K = shapes[0][1];
			const size_t Kb = shapes[1][0];
			const size_t N = shapes[1][1];

			if (K == Kb) {
				plan.matMulValid = true;
				plan.matMulBatched = false;
				plan.M = M;
				plan.K = K;
				plan.N = N;
				plan.matMulResultShape = { M, N };
			}
		}

		return plan;
	}


	template BroadcastPlan CreateBroadcastPlan<float>(const std::vector<TensorView<float>>& tensors);

	bool CanBroadcast(const std::vector<size_t>& shapeA, const std::vector<size_t>& shapeB, std::vector<size_t>& resultShape) {
		resultShape.clear();
		bool canBroadcast = true;
		const std::vector<size_t>& maxShape = (shapeA.size() > shapeB.size()) ? shapeA : shapeB;
		const std::vector<size_t>& broadcastShape = (shapeA.size() > shapeB.size()) ? shapeB : shapeA;
		size_t currentDim = 0;
		for (auto i = maxShape.rbegin(); i != maxShape.rend(); i++) {
			if (currentDim < broadcastShape.size()) {
				size_t a = *i;
				size_t b = broadcastShape[broadcastShape.size() - 1 - currentDim];
				if (a != b && a != 1 && b != 1) {
					canBroadcast = false;
					break;
				}
				resultShape.insert(resultShape.begin(), (std::max)(a, b));
			}
			else {
				resultShape.insert(resultShape.begin(), *i);
			}
			currentDim++;
		}
		return canBroadcast;
	}

	bool CanBroadcast(const std::vector<std::vector<size_t>>& shapes, std::vector<size_t>& resultShape) {
		resultShape.clear();
		if (shapes.empty()) return false;

		std::vector<size_t> temp;

		resultShape = shapes[0];

		for (size_t i = 1; i < shapes.size(); i++) {
			if (!CanBroadcast(resultShape, shapes[i], temp)) 
				return false;
			
			resultShape = temp;
		}


		return true;
	}

	bool CanBroadcastMatMul(const std::vector<std::vector<size_t>>& shapes, std::vector<size_t>& resultShape)
	{
		// For now only check if the last two dimensions are compatible for matrix multiplication
		if (shapes.size() != 2) return false;
		if (shapes[0].size() == 2 && shapes[1].size() == 2) return true;

		//if (shapes[0].size() < 2 || shapes[1].size() < 2) return false;


		return false;
	}

	void GetEffectiveStrides(const std::vector<size_t>& shape, std::vector<size_t>& strides, const std::vector<size_t>& resultShape) {
		size_t i = 0;
		for (size_t& stride : strides) {
			if (shape.size() < i + 1 || (shape[i] == 1 && resultShape[i] != 1)) stride = 0;
			i++;
		}


		while (strides.size() < resultShape.size()) {
			strides.insert(strides.begin(), 0);
		}
	}

	template<typename T>
	void AddContinious(const TensorView<T>& a, const TensorView<T>& b, TensorView<T>& result) {
		T* aData = const_cast<T*>(a.getDataPtr());
		T* bData = const_cast<T*>(b.getDataPtr());
		T* cData = const_cast<T*>(result.getDataPtr());

		uint32_t width = 32 / sizeof(T);

		size_t i = 0;

		for (; i + width <= result.getTotalSize(); i += width) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecB = _mm256_loadu_ps(&bData[i]);
			__m256 vecResult = _mm256_add_ps(vecA, vecB);
			_mm256_storeu_ps(&cData[i], vecResult);
		}

		for (; i < result.getTotalSize(); i++) {
			cData[i] = aData[i] + bData[i];
		}

	}

	template <typename T>
	void MulContinious(const TensorView<T>& a, const TensorView<T>& b, TensorView<T>& result) {
		T* aData = const_cast<T*>(a.getDataPtr());
		T* bData = const_cast<T*>(b.getDataPtr());
		T* cData = const_cast<T*>(result.getDataPtr());
		uint32_t width = 32 / sizeof(T);
		size_t i = 0;

		for (; i + width <= result.getTotalSize(); i += width) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecB = _mm256_loadu_ps(&bData[i]);
			__m256 vecResult = _mm256_mul_ps(vecA, vecB);
			_mm256_storeu_ps(&cData[i], vecResult);
		}
		for (; i < result.getTotalSize(); i++) {
			cData[i] = aData[i] * bData[i];
		}
	}

	template <typename T>
	void MulAddContinious(const TensorView<T>& a, const TensorView<T>& b, const TensorView<T>& c, TensorView<T>& result) {
		T* aData = const_cast<T*>(a.getDataPtr());
		T* bData = const_cast<T*>(b.getDataPtr());
		T* cData = const_cast<T*>(c.getDataPtr());
		T* resultData = const_cast<T*>(result.getDataPtr());


		uint32_t width = 32 / sizeof(T);
		size_t i = 0;
		for (; i + width <= result.getTotalSize(); i += width) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecB = _mm256_loadu_ps(&bData[i]);
			__m256 vecC = _mm256_loadu_ps(&cData[i]);
			__m256 vecResult = _mm256_fmadd_ps(vecA, vecB, vecC);
			_mm256_storeu_ps(&resultData[i], vecResult);
		}

		for (; i < result.getTotalSize(); i++) {
			resultData[i] = aData[i] * bData[i] + cData[i];
		}
	}

	template void AddContinious<float>(const TensorView<float>& a, const TensorView<float>& b, TensorView<float>& result);
	template void MulContinious<float>(const TensorView<float>& a, const TensorView<float>& b, TensorView<float>& result);
	template void MulAddContinious<float>(const TensorView<float>& a, const TensorView<float>& b, const TensorView<float>& c, TensorView<float>& result);



	template<typename T>
	void IterateOverBroadcastedPlan(const std::vector<TensorView<T>>& tensors, size_t totalSize, const BroadcastPlan& plan, std::function<void(const std::vector<size_t>&, size_t)> func) {
		std::vector<size_t> coords(plan.resultShape.size(), 0);
		std::vector<size_t> indexes(tensors.size(), 0);
		
		for (size_t i = 0; i < totalSize; i++) {
			func(indexes, i);
			for (size_t d = plan.resultShape.size(); d-- > 0;) {
				coords[d]++;
				for (size_t t = 0; t < tensors.size(); t++) {
					indexes[t] += plan.effectiveStrides[t][d];
				}
				if (coords[d] < plan.resultShape[d])
					break;
				coords[d] = 0;
				for (size_t t = 0; t < tensors.size(); t++) {
					indexes[t] -= plan.effectiveStrides[t][d] * plan.resultShape[d];
				}
			}
		}
	}

	template void IterateOverBroadcastedPlan<float>(const std::vector<TensorView<float>>& tensors, size_t totalSize, const BroadcastPlan& plan, std::function<void(const std::vector<size_t>&, size_t)> func);

	template<typename T>
	void Add(const TensorView<T>& a, const TensorView<T>& b, TensorView<T>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid.");

		if (plan.continious) {
			AddContinious<T>(a, b, result);
			return;
		}

		assert(result.getShape() == plan.resultShape && "Result tensor shape must match the expected broadcasted shape.");

		IterateOverBroadcastedPlan<T>({a, b}, result.getTotalSize(), plan, [&](const std::vector<size_t>& indexes, size_t resultIndex) {
			result[resultIndex] = a[indexes[0]] + b[indexes[1]];
			});
	}


	template<typename T>
	void Mul(const TensorView<T>& a, const TensorView<T>& b, TensorView<T>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid.");

		if (plan.continious) {
			MulContinious<T>(a, b, result);
			return;
		}
		assert(result.getShape() == plan.resultShape && "Result tensor shape must match the expected broadcasted shape.");
		IterateOverBroadcastedPlan<T>({a, b}, result.getTotalSize(), plan, [&](const std::vector<size_t>& indexes, size_t resultIndex) {
			result[resultIndex] = a[indexes[0]] * b[indexes[1]];
			});
	}

	template<typename T>
	void MulAdd(const TensorView<T>& a, const TensorView<T>& b, const TensorView<T>& c, TensorView<T>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid.");

		if (plan.continious) {
			MulAddContinious<T>(a, b, c, result);
			return;
		}
		assert(result.getShape() == plan.resultShape && "Result tensor shape must match the expected broadcasted shape.");
		IterateOverBroadcastedPlan<T>({a, b, c}, result.getTotalSize(), plan, [&](const std::vector<size_t>& indexes, size_t resultIndex) {
			result[resultIndex] = a[indexes[0]] * b[indexes[1]] + c[indexes[2]];
			});
	}

	template<typename T>
	void SiLU(const Tensor<T>& a, Tensor<T>& result)
	{
		
		for (size_t i = 0; i < a.getTotalSize(); ++i) {
			result[i] = a[i] / (1.0f + std::exp(-a[i]));
		}

	}


	template void Add<float>(const TensorView<float>& a, const TensorView<float>& b, TensorView<float>& result, const BroadcastPlan& plan);
	template void Mul<float>(const TensorView<float>& a, const TensorView<float>& b, TensorView<float>& result, const BroadcastPlan& plan);
	template void MulAdd<float>(const TensorView<float>& a, const TensorView<float>& b, const TensorView<float>& c, TensorView<float>& result, const BroadcastPlan& plan);
	template void SiLU<float>(const Tensor<float>& a, Tensor<float>& result);

}
