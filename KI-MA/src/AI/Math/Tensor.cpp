#include "Tensor.h"

#include <cassert>

namespace AI {

	template<typename T>
	TensorView<T>::TensorView(Tensor<T>& base, size_t offset, size_t viewSize, const std::vector<size_t>& shape)
	{
		m_BaseTensor = &base;
		m_Offset = offset;
		m_ViewSize = viewSize;

		uint32_t size = 1;
		for (auto dimSize : shape)
			size *= dimSize;

		assert(size == base.getTotalSize() && "View size must not exceed base tensor size");

		m_ViewShape = shape;
		m_ViewStrides.resize(shape.size());
		size_t stride = 1;
		for (size_t i = shape.size(); i > 0; i--)
		{
			m_ViewStrides[i - 1] = stride;
			stride *= shape[i - 1];
		}

	}

	template<typename T>
	TensorView<T>::~TensorView()
	{
		
	}

	template<typename T>
	Tensor<T>::Tensor(const std::vector<size_t>& shape)
	{
		setShape(shape);

		m_TotalSize = 1;
		for (auto dimSize : shape)
			m_TotalSize *= dimSize;

		m_DataPtr = new T[m_TotalSize];
		m_CachedPackedDataPtr = nullptr;
	}

	template<typename T>
	Tensor<T>::~Tensor()
	{
		delete[] m_DataPtr;

		if (m_CachedPackedDataPtr)
			delete[] m_CachedPackedDataPtr;
	}

	template<typename T>
	void Tensor<T>::genPackedData(uint32_t NSize, uint32_t KSize)
	{
		m_CachedPackedDataPtr = new T[m_TotalSize];

		float* dstPtr = m_CachedPackedDataPtr;
		size_t n = 0;
		for (; n + 16 <= NSize; n += 16) {
			const float* src = m_DataPtr + n;

			for (size_t k = 0; k < KSize; k++) {
				__m256 src1 = _mm256_load_ps(src);
				__m256 src2 = _mm256_load_ps(src + 8);

				_mm256_store_ps(dstPtr, src1);
				_mm256_store_ps(dstPtr + 8, src2);

				src += NSize;
				dstPtr += 16;
			}
		}
	}

	template<typename T>
	void Tensor<T>::setShape(const std::vector<size_t>& shape)
	{
		m_Shape = shape;
		m_Strides.resize(shape.size());

		size_t stride = 1;

		for(size_t i = shape.size(); i > 0; i--)
		{
			m_Strides[i - 1] = stride;
			stride *= shape[i - 1];
		}

	}

	template class Tensor<float>;

	template class TensorView<float>;
}
