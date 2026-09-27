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
	}

	template<typename T>
	Tensor<T>::~Tensor()
	{
		delete[] m_DataPtr;
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
