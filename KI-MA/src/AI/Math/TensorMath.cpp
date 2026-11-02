#include "TensorMath.h"

#include <assert.h>

#include <immintrin.h>
#include <functional>
#include <algorithm>
#include <execution>
#include <ranges>

namespace AI::Math {
	BroadcastPlan CreateBroadcastPlan(const std::vector<Tensor<float>*>& tensors)
	{
		BroadcastPlan plan;

		std::vector<std::vector<size_t>> shapes;
		shapes.reserve(tensors.size());
		plan.effectiveStrides.reserve(tensors.size());

		for (const auto& tensor : tensors) {
			shapes.push_back(tensor->getShape());
			plan.effectiveStrides.push_back(tensor->getStrides());
		}

		if (shapes.size() == 1 || std::all_of(shapes.begin(), shapes.end(), [&](const std::vector<size_t>& shape) { return shape == shapes[0]; })) {
			plan.continuous = true;
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
				resultShape.insert(resultShape.begin(), std::max(a, b));
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

	void AddContinuous(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* bData = const_cast<float*>(b.getDataPtr());
		float* cData = const_cast<float*>(result.getDataPtr());

		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecB = _mm256_loadu_ps(&bData[i]);
			__m256 vecResult = _mm256_add_ps(vecA, vecB);
			_mm256_storeu_ps(&cData[i], vecResult);
		}
	}

	void AddContinuousScalar(const Tensor<float>& a, const float& scalar, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* cData = const_cast<float*>(result.getDataPtr());

		__m256 vecScalar = _mm256_set1_ps(scalar);

		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecResult = _mm256_add_ps(vecA, vecScalar);
			_mm256_storeu_ps(&cData[i], vecResult);
		}
	}

	void SubContinuous(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* bData = const_cast<float*>(b.getDataPtr());
		float* cData = const_cast<float*>(result.getDataPtr());
		
		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecB = _mm256_loadu_ps(&bData[i]);
			__m256 vecResult = _mm256_sub_ps(vecA, vecB);
			_mm256_storeu_ps(&cData[i], vecResult);
		}
	}

	void SubContinuousScalar(const Tensor<float>& a, const float& scalar, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* cData = const_cast<float*>(result.getDataPtr());
		__m256 vecScalar = _mm256_set1_ps(scalar);

		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecResult = _mm256_sub_ps(vecA, vecScalar);
			_mm256_storeu_ps(&cData[i], vecResult);
		}
	}

	void MulContinuous(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* bData = const_cast<float*>(b.getDataPtr());
		float* cData = const_cast<float*>(result.getDataPtr());
		
		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecB = _mm256_loadu_ps(&bData[i]);
			__m256 vecResult = _mm256_mul_ps(vecA, vecB);
			_mm256_storeu_ps(&cData[i], vecResult);
		}
	}

	void MulContinuousScalar(const Tensor<float>& a, const float& scalar, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* cData = const_cast<float*>(result.getDataPtr());
		__m256 vecScalar = _mm256_set1_ps(scalar);

		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecResult = _mm256_mul_ps(vecA, vecScalar);
			_mm256_storeu_ps(&cData[i], vecResult);
		}
	}

	void MulAddContinuous(const Tensor<float>& a, const Tensor<float>& b, const Tensor<float>& c, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* bData = const_cast<float*>(b.getDataPtr());
		float* cData = const_cast<float*>(c.getDataPtr());
		float* resultData = const_cast<float*>(result.getDataPtr());


		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecB = _mm256_loadu_ps(&bData[i]);
			__m256 vecC = _mm256_loadu_ps(&cData[i]);
			__m256 vecResult = _mm256_fmadd_ps(vecA, vecB, vecC);
			_mm256_storeu_ps(&resultData[i], vecResult);
		}
	}

	void MulAddContinuousScalar(const Tensor<float>& a, const float& scalar, const Tensor<float>& c, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* cData = const_cast<float*>(c.getDataPtr());
		float* resultData = const_cast<float*>(result.getDataPtr());
		__m256 vecScalar = _mm256_set1_ps(scalar);

		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecC = _mm256_loadu_ps(&cData[i]);
			__m256 vecResult = _mm256_fmadd_ps(vecA, vecScalar, vecC);
			_mm256_storeu_ps(&resultData[i], vecResult);
		}
	}

	void MulAddContinuousScalar(const Tensor<float>& a, const Tensor<float>& b, const float& scalar, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* bData = const_cast<float*>(b.getDataPtr());
		float* resultData = const_cast<float*>(result.getDataPtr());
		__m256 vecScalar = _mm256_set1_ps(scalar);

		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecB = _mm256_loadu_ps(&bData[i]);
			__m256 vecResult = _mm256_fmadd_ps(vecA, vecB, vecScalar);
			_mm256_storeu_ps(&resultData[i], vecResult);
		}
	}

	void MulAddContinuousScalar(const Tensor<float>& a, const float& scalar, const float& c, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* resultData = const_cast<float*>(result.getDataPtr());
		__m256 vecScalar = _mm256_set1_ps(scalar);
		__m256 vecC = _mm256_set1_ps(c);

		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecResult = _mm256_fmadd_ps(vecA, vecScalar, vecC);
			_mm256_storeu_ps(&resultData[i], vecResult);
		}
	}

	void DivContinuous(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* bData = const_cast<float*>(b.getDataPtr());
		float* cData = const_cast<float*>(result.getDataPtr());

		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecB = _mm256_loadu_ps(&bData[i]);
			__m256 vecResult = _mm256_div_ps(vecA, vecB);
			_mm256_storeu_ps(&cData[i], vecResult);
		}
	}

	void DivContinuousScalar(const Tensor<float>& a, const float& scalar, Tensor<float>& result) {
		float* aData = const_cast<float*>(a.getDataPtr());
		float* cData = const_cast<float*>(result.getDataPtr());
		__m256 vecScalar = _mm256_set1_ps(scalar);

		for (size_t i = 0; i < result.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_loadu_ps(&aData[i]);
			__m256 vecResult = _mm256_div_ps(vecA, vecScalar);
			_mm256_storeu_ps(&cData[i], vecResult);
		}
	}

	template <typename Func>
	void IterateOverBroadcastedPlan(const std::vector<Tensor<float>>& tensors, size_t totalSize, const BroadcastPlan& plan, Func&& func) {
		size_t dimensions = plan.resultShape.size();

		std::vector<size_t> coords(dimensions, 0);
		std::vector<size_t> indexes(tensors.size(), 0);
		
		for (size_t i = 0; i < totalSize; i++) {
			func(indexes, i);

			if(i == totalSize - 1)
				break;

			for (size_t d = dimensions; d-- > 0;) {
				coords[d]++;

				if (coords[d] < plan.resultShape[d]) {
					for (size_t t = 0; t < tensors.size(); t++) {
						indexes[t] += plan.effectiveStrides[t][d];
					}
					break;
				}

				coords[d] = 0;
				for (size_t t = 0; t < tensors.size(); t++) {
					indexes[t] -= plan.effectiveStrides[t][d] * (plan.resultShape[d] - 1);
				}
			}
		}
	}

	void Add(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid.");
		if (a.isCPUDataDirty()) a.updateCPUData();
		if (b.isCPUDataDirty()) b.updateCPUData();

		if (plan.continuous) {
			AddContinuous(a, b, result);
			result.markCPUWritten();
			return;
		}

		if (b.getTotalSize() == 1) {
			AddContinuousScalar(a, b[0], result);
			result.markCPUWritten();
			return;
		}

		assert(result.getShape() == plan.resultShape && "Result tensor shape must match the expected broadcasted shape.");

		IterateOverBroadcastedPlan({a, b}, result.getTotalSize(), plan, [&](const std::vector<size_t>& indexes, size_t resultIndex) {
			result[resultIndex] = a[indexes[0]] + b[indexes[1]];
			});

		result.markCPUWritten();
	}

	void Sub(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan)
	{
		assert(plan.valid && "Broadcast plan is not valid.");
		if (a.isCPUDataDirty()) a.updateCPUData();
		if (b.isCPUDataDirty()) b.updateCPUData();

		if (plan.continuous) {
			SubContinuous(a, b, result);
			result.markCPUWritten();
			return;
		}

		if (b.getTotalSize() == 1) {
			SubContinuousScalar(a, b[0], result);
			result.markCPUWritten();
			return;
		}

		assert(result.getShape() == plan.resultShape && "Result tensor shape must match the expected broadcasted shape.");

		IterateOverBroadcastedPlan({ a, b }, result.getTotalSize(), plan, [&](const std::vector<size_t>& indexes, size_t resultIndex) {
			result[resultIndex] = a[indexes[0]] - b[indexes[1]];
			});

		result.markCPUWritten();
	}


	void Mul(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid.");
		if (a.isCPUDataDirty()) a.updateCPUData();
		if (b.isCPUDataDirty()) b.updateCPUData();

		if (plan.continuous) {
			MulContinuous(a, b, result);
			result.markCPUWritten();
			return;
		}

		if (b.getTotalSize() == 1) {
			MulContinuousScalar(a, b[0], result);
			result.markCPUWritten();
			return;
		}

		assert(result.getShape() == plan.resultShape && "Result tensor shape must match the expected broadcasted shape.");
		IterateOverBroadcastedPlan({a, b}, result.getTotalSize(), plan, [&](const std::vector<size_t>& indexes, size_t resultIndex) {
			result[resultIndex] = a[indexes[0]] * b[indexes[1]];
			});

		result.markCPUWritten();
	}

	void MulAdd(const Tensor<float>& a, const Tensor<float>& b, const Tensor<float>& c, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.valid && "Broadcast plan is not valid.");
		if (a.isCPUDataDirty()) a.updateCPUData();
		if (b.isCPUDataDirty()) b.updateCPUData();
		if (c.isCPUDataDirty()) c.updateCPUData();

		if (plan.continuous) {
			MulAddContinuous(a, b, c, result);
			result.markCPUWritten();
			return;
		}

		if (b.getTotalSize() == 1 && c.getTotalSize() == 1) {
			MulAddContinuousScalar(a, b[0], c[0], result);
			result.markCPUWritten();
			return;
		}

		if (b.getTotalSize() == 1) {
			MulAddContinuousScalar(a, b[0], c, result);
			result.markCPUWritten();
			return;
		}

		if (c.getTotalSize() == 1) {
			MulAddContinuousScalar(a, b, c[0], result);
			result.markCPUWritten();
			return;
		}

		assert(result.getShape() == plan.resultShape && "Result tensor shape must match the expected broadcasted shape.");
		IterateOverBroadcastedPlan({a, b, c}, result.getTotalSize(), plan, [&](const std::vector<size_t>& indexes, size_t resultIndex) {
			result[resultIndex] = a[indexes[0]] * b[indexes[1]] + c[indexes[2]];
			});
		result.markCPUWritten();
	}

	void Div(const Tensor<float>& a, const Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan)
	{
		assert(plan.valid && "Broadcast plan is not valid.");
		if (a.isCPUDataDirty()) a.updateCPUData();
		if (b.isCPUDataDirty()) b.updateCPUData();

		if (plan.continuous) {
			DivContinuous(a, b, result);
			result.markCPUWritten();
			return;
		}

		if (b.getTotalSize() == 1) {
			DivContinuousScalar(a, b[0], result);
			result.markCPUWritten();
			return;
		}

		assert(result.getShape() == plan.resultShape && "Result tensor shape must match the expected broadcasted shape.");

		IterateOverBroadcastedPlan({ a, b }, result.getTotalSize(), plan, [&](const std::vector<size_t>& indexes, size_t resultIndex) {
			result[resultIndex] = a[indexes[0]] / b[indexes[1]];
			});

		result.markCPUWritten();
	}


	void SiLU(const Tensor<float>& a, Tensor<float>& result)
	{
		if(a.isCPUDataDirty()) a.updateCPUData();

		for (size_t i = 0; i < a.getTotalSize(); ++i) {
			result[i] = a[i] / (1.0f + std::exp(-a[i]));
		}
		result.markCPUWritten();
	}

	void SiLUGradient(const Tensor<float>& a, Tensor<float>& inputGradient, Tensor<float>& outputGradient)
	{
		if (a.isCPUDataDirty()) a.updateCPUData();
		if (inputGradient.isCPUDataDirty()) inputGradient.updateCPUData();

		for(size_t i = 0; i < a.getTotalSize(); ++i) {
			float sigmoid = 1.0f / (1.0f + std::exp(-a[i]));
			float derivative = sigmoid + a[i] * sigmoid * (1.0f - sigmoid);
			outputGradient[i] = derivative * inputGradient[i];
		}
		outputGradient.markCPUWritten();
	}

	void ReLU(const Tensor<float>& a, Tensor<float>& result)
	{
		if (a.isCPUDataDirty()) a.updateCPUData();

		for (size_t i = 0; i < a.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_load_ps(&a[i]);
			vecA = _mm256_max_ps(vecA, _mm256_setzero_ps());
			_mm256_store_ps(&result[i], vecA);
		}
		result.markCPUWritten();
	}

	void ReLUGradient(const Tensor<float>& a, Tensor<float>& inputGradient, Tensor<float>& outputGradient)
	{
		if (a.isCPUDataDirty()) a.updateCPUData();
		if (inputGradient.isCPUDataDirty()) inputGradient.updateCPUData();

		for (size_t i = 0; i < a.getTotalSize(); i += 8) {
			__m256 vecA = _mm256_load_ps(&a[i]);
			__m256 vecGrad = _mm256_load_ps(&inputGradient[i]);
			__m256 mask = _mm256_cmp_ps(vecA, _mm256_setzero_ps(), _CMP_GT_OS);
			__m256 result = _mm256_and_ps(vecGrad, mask);
			_mm256_store_ps(&outputGradient[i], result);
		}
		outputGradient.markCPUWritten();
	}

	// 1 / 2n * sum((results - targets)^2)
	void MSELoss(const Tensor<float>& results, const Tensor<float>& targets, float& loss) {
		assert(results.getTotalSize() == targets.getTotalSize() && "Results and targets must have the same size.");
		
		if (results.isCPUDataDirty()) results.updateCPUData();
		if (targets.isCPUDataDirty()) targets.updateCPUData();
		
		size_t n = results.getTotalSize();

		float sum = 0.0f;
		for (size_t i = 0; i < n; ++i) {
			float diff = results[i] - targets[i];
			sum += diff * diff;
		}
		loss = sum / (2.0f * static_cast<float>(n));
	}

	void MSELossGradient(const Tensor<float>& results, const Tensor<float>& targets, Tensor<float>& gradient) {
		assert(results.getTotalSize() == targets.getTotalSize() && "Results and targets must have the same size.");

		if (results.isCPUDataDirty()) results.updateCPUData();
		if (targets.isCPUDataDirty()) targets.updateCPUData();

		for (size_t i = 0; i < results.getTotalSize(); ++i) {
			gradient[i] = (results[i] - targets[i]) / static_cast<float>(results.getTotalSize());
		}
		gradient.markCPUWritten();
	}

	void Softmax(const Tensor<float>& input, Tensor<float>& output)
	{
		assert(input.getShape().size() == 2 && "Softmax only supports 2D tensors.");
		assert(input.getShape() == output.getShape() && "Input and output must have the same shape.");

		if (input.isCPUDataDirty()) input.updateCPUData();

		size_t batchSize = input.getShape()[0];
		size_t classes = input.getShape()[1];
		
		for (size_t s = 0; s < batchSize; s++) {
			size_t offset = s * classes;

			float maxVal = input[offset];
			for (size_t c = 1; c < classes; c++) 
				maxVal = std::max(maxVal, input[offset + c]);
			

			float sumExp = 0.0f;
			for (size_t c = 0; c < classes; c++) {
				output[offset + c] = std::exp(input[offset + c] - maxVal);
				sumExp += output[offset + c];
			}

			for (size_t c = 0; c < classes; c++) {
				output[offset + c] /= sumExp;
			}
		}
		output.markCPUWritten();
	}

	void CELoss(const Tensor<float>& results, const Tensor<float>& targets, float& loss)
	{
		assert(results.getShape().size() == 2);
		assert(results.getShape() == targets.getShape());
		
		if (results.isCPUDataDirty()) results.updateCPUData();
		if (targets.isCPUDataDirty()) targets.updateCPUData();

		size_t batchSize = results.getShape()[0];
		loss = 0;
		for (size_t i = 0; i < results.getTotalSize(); i++)
		{
			float p = std::max(1e-9f, static_cast<float>(results[i])); // Clamp to avoid log(0) and log(1)
			loss -= targets[i] * std::log(p); // Use the clamped value
		}

		loss /= static_cast<float>(batchSize);

	}

	void  CELossGradient(const Tensor<float>& results, const Tensor<float>& targets, Tensor<float>& gradient)
	{
		assert(results.getTotalSize() == targets.getTotalSize() && "Results and targets must have the same size.");

		if (results.isCPUDataDirty()) results.updateCPUData();
		if (targets.isCPUDataDirty()) targets.updateCPUData();

		for (size_t i = 0; i < results.getTotalSize(); ++i) {
			gradient[i] = (results[i] - targets[i]) / results.getShape()[0];
		}

		gradient.markCPUWritten();
	}

	//https://airccse.org/journal/jcsit/6314ijcsit05.pdf
	inline void Transpose8x8(const float* src, float* dst, size_t srcStride, size_t dstStride) {
		__m256 r0 = _mm256_load_ps(src + 0 * srcStride);
		__m256 r1 = _mm256_load_ps(src + 1 * srcStride);
		__m256 r2 = _mm256_load_ps(src + 2 * srcStride);
		__m256 r3 = _mm256_load_ps(src + 3 * srcStride);
		__m256 r4 = _mm256_load_ps(src + 4 * srcStride);
		__m256 r5 = _mm256_load_ps(src + 5 * srcStride);
		__m256 r6 = _mm256_load_ps(src + 6 * srcStride);
		__m256 r7 = _mm256_load_ps(src + 7 * srcStride);

		const __m256 t0 = _mm256_unpacklo_ps(r0, r1);
		const __m256 t1 = _mm256_unpackhi_ps(r0, r1);
		const __m256 t2 = _mm256_unpacklo_ps(r2, r3);
		const __m256 t3 = _mm256_unpackhi_ps(r2, r3);
		const __m256 t4 = _mm256_unpacklo_ps(r4, r5);
		const __m256 t5 = _mm256_unpackhi_ps(r4, r5);
		const __m256 t6 = _mm256_unpacklo_ps(r6, r7);
		const __m256 t7 = _mm256_unpackhi_ps(r6, r7);

		const __m256 s0 = _mm256_shuffle_ps(t0, t2, 0x44);
		const __m256 s1 = _mm256_shuffle_ps(t0, t2, 0xEE);
		const __m256 s2 = _mm256_shuffle_ps(t1, t3, 0x44);
		const __m256 s3 = _mm256_shuffle_ps(t1, t3, 0xEE);
		const __m256 s4 = _mm256_shuffle_ps(t4, t6, 0x44);
		const __m256 s5 = _mm256_shuffle_ps(t4, t6, 0xEE);
		const __m256 s6 = _mm256_shuffle_ps(t5, t7, 0x44);
		const __m256 s7 = _mm256_shuffle_ps(t5, t7, 0xEE);

		_mm256_store_ps(dst + 0 * dstStride, _mm256_permute2f128_ps(s0, s4, 0x20));
		_mm256_store_ps(dst + 1 * dstStride, _mm256_permute2f128_ps(s1, s5, 0x20));
		_mm256_store_ps(dst + 2 * dstStride, _mm256_permute2f128_ps(s2, s6, 0x20));
		_mm256_store_ps(dst + 3 * dstStride, _mm256_permute2f128_ps(s3, s7, 0x20));
		_mm256_store_ps(dst + 4 * dstStride, _mm256_permute2f128_ps(s0, s4, 0x31));
		_mm256_store_ps(dst + 5 * dstStride, _mm256_permute2f128_ps(s1, s5, 0x31));
		_mm256_store_ps(dst + 6 * dstStride, _mm256_permute2f128_ps(s2, s6, 0x31));
		_mm256_store_ps(dst + 7 * dstStride, _mm256_permute2f128_ps(s3, s7, 0x31));
	};

	void Transpose(Tensor<float>& input, Tensor<float>& output)
	{
		//Tensor<float> temp(input.getShape());
		assert(input.getShape().size() == 2 && output.getShape().size() == 2 && "Only 2D tensors are supported for transpose.");
		assert(input.getShape()[0] == output.getShape()[1] && input.getShape()[1] == output.getShape()[0] && "Output tensor shape must be the transpose of the input tensor shape.");
		
		if (input.isCPUDataDirty()) input.updateCPUData();

		output.clearCachedData();

		const size_t rows = input.getShape()[0];
		const size_t cols = input.getShape()[1];
		constexpr size_t blockSize = 32;
	
		for (size_t r0 = 0; r0 < rows; r0 += blockSize) {
			for (size_t c0 = 0; c0 < cols; c0 += blockSize) {
				size_t rMax = std::min(r0 + blockSize, rows);
				size_t cMax = std::min(c0 + blockSize, cols);
				
				size_t r = r0;
				for (; r + 8 <= rMax; r += 8) {
					size_t c = c0;

					for (; c + 8 <= cMax; c += 8) {
						Transpose8x8(input.getDataPtr() + r * cols + c, output.getDataPtr() + c * rows + r, cols, rows);
					}

					for (; c < cMax; ++c) {
						for (size_t rr = r; rr < r + 8; rr++) {
							output[c * rows + rr] = input[rr * cols + c];
						}
					}

				}

				for (; r < rMax; ++r) {
					for (size_t c = c0; c < cMax; ++c) {
						output[c * rows + r] = input[r * cols + c];
					}
				}

			}
		}

		output.markCPUWritten();
	}

	void Im2Col(const Tensor<float>& input, Tensor<float>& output, size_t kH, size_t kW, size_t sH, size_t sW, size_t pH, size_t pW) {
		assert(input.getShape().size() == 4 && "Input must be a 4D tensor");

		// Input is [B, H, W, C]
		// Output is [B * outH * outW, C * kH * kW]

		size_t B = input.getShape()[0];
		size_t H = input.getShape()[1];
		size_t W = input.getShape()[2];
		size_t C = input.getShape()[3];

		size_t outH = (H + 2 * pH - kH) / sH + 1;
		size_t outW = (W + 2 * pW - kW) / sW + 1;

		if (input.isCPUDataDirty()) input.updateCPUData();

		if (output.getTotalSize() != B * outH * outW * C * kH * kW) {
			output = Tensor<float>({ B * outH * outW, C * kH * kW });
		}
		
		for (size_t b = 0; b < B; b++) {
			for (size_t outY = 0; outY < outH; outY++) {
				for (size_t outX = 0; outX < outW; outX++) {
					for (size_t kY = 0; kY < kH; kY++) {
						for (size_t kX = 0; kX < kW; kX++) {
							for (size_t c = 0; c < C; c++) {
								int64_t iX = int64_t(outX * sW + kX) - pW;
								int64_t iY = int64_t(outY * sH + kY) - pH;

								size_t row = (b * outH + outY) * outW + outX;
								size_t col = (kY * kW + kX) * C + c;

								float value = 0.0f;
								if (iY >= 0 && iY < int64_t(H) && iX >= 0 && iX < int64_t(W))
									value = input[((b * H + iY) * W + iX) * C + c];
	
								output.getDataPtr()[row * (kH * kW * C) + col] = value;

							}
						}
					}
				}
			}
		}

		output.markCPUWritten();
	}

}
