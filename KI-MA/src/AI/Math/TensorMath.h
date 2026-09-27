#pragma once

#include "Tensor.h"

namespace AI::Math {

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

	
	void GetEffectiveStrides(const std::vector<size_t>& shape, std::vector<size_t>& strides, const std::vector<size_t>& resultShape) {

		size_t i = 0;
		for (size_t& stride : strides) {
			if (shape.size() < i + 1 || (shape[i] == 1 && resultShape[i] != 1)) stride = 0;
			i++;
		}


		while(strides.size() < resultShape.size()) {
			strides.insert(strides.begin(), 0);
		}


	}

	template<typename T>
	void Add(const TensorView<T>& a, const TensorView<T>& b, TensorView<T>& result) {
		const std::vector<size_t>& shapeA = a.getShape();
		const std::vector<size_t>& shapeB = b.getShape();
		const std::vector<size_t>& shapeResult = result.getShape();

		// Broadcasting
		std::vector<size_t> expectedResultShape;
		bool broadcastable = CanBroadcast(shapeA, shapeB, expectedResultShape);

		assert(broadcastable && "Tensors cannot be broadcasted to a common shape.");
		assert(shapeResult == expectedResultShape && "Result tensor shape must match the expected broadcasted shape.");

		std::vector<size_t> effectiveStridesA = a.getStrides();
		std::vector<size_t> effectiveStridesB = b.getStrides();

		const std::vector<size_t>& resultStrides = result.getStrides();

		GetEffectiveStrides(shapeA, effectiveStridesA, shapeResult);
		GetEffectiveStrides(shapeB, effectiveStridesB, shapeResult);

		for(size_t i = 0; i < result.getTotalSize(); i++) {
			size_t indexA = 0;
			size_t indexB = 0;

			size_t carry = i;
			for (size_t j = 0; j < shapeResult.size(); j++)
			{
				size_t coordinate = carry / resultStrides[j];
				carry %= resultStrides[j];

				indexA += coordinate * effectiveStridesA[j];
				indexB += coordinate * effectiveStridesB[j];

			}

			result[i] = a[indexA] + b[indexB];
		}	
	}
}