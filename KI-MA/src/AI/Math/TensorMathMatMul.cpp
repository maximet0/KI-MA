#include "TensorMath.h"
#include "Core/Logger.h"

#include <assert.h>
#include <functional>
#include <algorithm>
#include <execution>

namespace AI::Math {
	//Optimierte GEMM (General Matrix-Matrix Multiplication) Kernels
	extern "C" void Kernel6x16_F32(const float* a[6], const float* packedB, float* c[6], size_t K);
	extern "C" void Kernel1x64_F32(const float* a, const float* packedB, float* c, size_t K);

	void MatMulNTail(const float* aData, const float* bData, float* resultData, size_t n, size_t m, size_t nSize, size_t kSize) {
		for (; n < nSize; ++n)
		{
			float sum = 0;
			for (size_t k = 0; k < kSize; ++k)
			{
				sum += aData[m * kSize + k] * bData[k * nSize + n];
			}
			resultData[m * nSize + n] = sum;
		}
	}

	void MatMulMTail(const float* aData, const float* bData, float* resultData, size_t m, size_t mSize, size_t nSize, size_t kSize) {
		for (; m < mSize; ++m)
		{
			for (size_t n = 0; n < nSize; ++n)
			{
				float sum = 0;
				for (size_t k = 0; k < kSize; ++k)
				{
					sum += aData[m * kSize + k] * bData[k * nSize + n];
				}
				resultData[m * nSize + n] = sum;
			}
		}
	}

	void MatMul(const Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.matMulValid && "Broadcast plan is not valid for MatMul.");
		assert(result.getShape() == plan.matMulResultShape && "Result tensor shape must match planned shape.");

		if (a.isCPUDataDirty()) a.updateCPUData();
		if (b.isCPUDataDirty()) b.updateCPUData();

		if (plan.matMulValid) {			
			size_t mSize = plan.M;
			size_t kSize = plan.K;
			size_t nSize = plan.N;

			const float* aData = a.getDataPtr();
			const float* bData = b.getDataPtr();
			float* resultData = result.getDataPtr();

			const float* packedBVals = b.getCachedPackedDataPtr();
		
			if (!packedBVals) {
				b.genPackedData(nSize, kSize);
				packedBVals = b.getCachedPackedDataPtr();
			}

			size_t m = 0;
			if (nSize >= 16) {	
				for (; m + 6 <= mSize; m += 6) {
					const float* a[6];
					float* c[6];
					for (size_t i = 0; i < 6; ++i) {
						a[i] = aData + (m + i) * kSize;
						c[i] = resultData + (m + i) * nSize;
					}

					size_t n = 0;
					//6x16 blocks
					for (; n + 16 <= nSize; n += 16) {
						float* cTile[6];
						cTile[0] = c[0] + n;
						cTile[1] = c[1] + n;
						cTile[2] = c[2] + n;
						cTile[3] = c[3] + n;
						cTile[4] = c[4] + n;
						cTile[5] = c[5] + n;


						Kernel6x16_F32(a, packedBVals + n * kSize, cTile, kSize);
					}

					for(size_t i = 0; i < 6; ++i) {
						MatMulNTail(aData, bData, resultData, n, m + i, nSize, kSize);
					}
				}

				for (; m < mSize; ++m)
				{
					const float* a = aData + m * kSize;
					float* c = resultData + m * nSize;

					size_t n = 0;
					for (; n + 64 <= nSize; n += 64) {
						Kernel1x64_F32(a, packedBVals + n * kSize, c + n, kSize);
					}

					MatMulNTail(aData, bData, resultData, n, m, nSize, kSize);
				}
			}

			MatMulMTail(aData, bData, resultData, m, mSize, nSize, kSize);
		}
		result.markCPUWritten();
	}

	void MatMulNaive(const Tensor<float>& a, Tensor<float>& b, Tensor<float>& result, const BroadcastPlan& plan) {
		assert(plan.matMulValid && "Broadcast plan is not valid for MatMul.");
		assert(result.getShape() == plan.matMulResultShape && "Result tensor shape must match planned shape.");
		if (plan.matMulValid) {
			size_t mSize = plan.M;
			size_t kSize = plan.K;
			size_t nSize = plan.N;
			const float* aData = a.getDataPtr();
			const float* bData = b.getDataPtr();
			float* resultData = result.getDataPtr();
			for (size_t m = 0; m < mSize; ++m) {
				for (size_t n = 0; n < nSize; ++n) {
					float sum = 0.0f;
					for (size_t k = 0; k < kSize; ++k) {
						sum += aData[m * kSize + k] * bData[k * nSize + n];
					}
					resultData[m * nSize + n] = sum;
				}
			}
		}
	}

}
