#include "Tensor.h"

#include <cassert>
#include <cstdlib>

#include "Core/Application.h"

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

		assert(size < base.getTotalSize() && "View size must not exceed base tensor size");

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
	Tensor<T> TensorView<T>::viewCopy() const
	{
		Tensor<T> copy(m_ViewShape);
		if (!m_BaseTensor->isGPUDataDirty()) {
			Graphics::BufferManager& bufferManager = Core::Application::getApplication()->getRenderer()->getBufferManager();
			bufferManager.copy(m_BaseTensor->getGPUDataHandle(), copy.getGPUDataHandle(), m_Offset, 0, m_ViewSize);
		}
		else
			std::copy(getDataPtr(), getDataPtr() + m_ViewSize, copy.getDataPtr());

		return copy;
	}

	template<typename T>
	Tensor<T>::Tensor() {}

	template<typename T>
	Tensor<T>::Tensor(const std::vector<size_t>& shape)
	{
		setShape(shape);

		m_TotalSize = 1;
		for (auto dimSize : shape)
			m_TotalSize *= dimSize;

		size_t avxPaddedSize = ((m_TotalSize + 7) / 8) * 8;

		m_DataPtr = static_cast<T*>(_aligned_malloc(avxPaddedSize * sizeof(T), 32));
		m_CachedPackedDataPtr = nullptr;
	}

	template<typename T>
	Tensor<T>::~Tensor()
	{

		if (m_DataPtr)
			_aligned_free(m_DataPtr);
		if (m_CachedPackedDataPtr)
			_aligned_free(m_CachedPackedDataPtr);

		if (m_GPUDataHandle != 0) {
			Graphics::BufferManager& bufferManager = Core::Application::getApplication()->getRenderer()->getBufferManager();
			bufferManager.destroyBuffer(m_GPUDataHandle);
		}
	}

	template<typename T>
	void Tensor<T>::copyTo(Tensor<T>& target) {
		if (!m_GPUDataDirty) {
			Graphics::BufferManager& bufferManager = Core::Application::getApplication()->getRenderer()->getBufferManager();

			if (!target.gpuDataExists())
				target.createGPUData();

			bufferManager.copy(m_GPUDataHandle, target.m_GPUDataHandle);
			target.m_GPUDataDirty = false;
		}

		if (!m_CPUDataDirty) {
			std::copy(m_DataPtr, m_DataPtr + m_TotalSize, target.m_DataPtr);
			target.m_CPUDataDirty = false;
		}
		
		target.m_CPUDataDirty = m_CPUDataDirty;
		target.m_GPUDataDirty = m_GPUDataDirty;
	}

	template<typename T>
	Tensor<T> Tensor<T>::copy() const
	{
		Tensor<T> copy(m_Shape);
		
		if (!m_GPUDataDirty) {
			Graphics::BufferManager& bufferManager = Core::Application::getApplication()->getRenderer()->getBufferManager();

			copy.createGPUData();
			bufferManager.copy(m_GPUDataHandle, copy.m_GPUDataHandle);
		}

		if (!m_CPUDataDirty) {
			std::copy(m_DataPtr, m_DataPtr + m_TotalSize, copy.m_DataPtr);
		}

		copy.m_CPUDataDirty = m_CPUDataDirty;
		copy.m_GPUDataDirty = m_GPUDataDirty;

		return copy;
	}

	template<typename T>
	void Tensor<T>::genPackedData(uint32_t NSize, uint32_t KSize)
	{
		if (!m_CachedPackedDataPtr)
			m_CachedPackedDataPtr = static_cast<T*>(_aligned_malloc(m_TotalSize * sizeof(T), 32));

		float* dstPtr = m_CachedPackedDataPtr;
		size_t n = 0;
		for (; n + 16 <= NSize; n += 16) {
			const float* src = m_DataPtr + n;

			for (size_t k = 0; k < KSize; k++) {
				__m256 src1 = _mm256_loadu_ps(src);
				__m256 src2 = _mm256_loadu_ps(src + 8);

				_mm256_storeu_ps(dstPtr, src1);
				_mm256_storeu_ps(dstPtr + 8, src2);

				src += NSize;
				dstPtr += 16;
			}
		}
		m_CachedPackedDataValid = true;
	}

	template<typename T>
	void Tensor<T>::clearCachedData()
	{
		m_CachedPackedDataValid = false;
		m_MinValue = std::numeric_limits<float>::max();
		m_MaxValue = std::numeric_limits<float>::lowest();
	}

	template<typename T>
	void Tensor<T>::updateGPUData() {
		Graphics::BufferManager& bufferManager = Core::Application::getApplication()->getRenderer()->getBufferManager();

		if (m_GPUDataHandle == 0) 
			m_GPUDataHandle = bufferManager.createBuffer(false, m_TotalSize * sizeof(T));
		
		bufferManager.write(m_GPUDataHandle, m_DataPtr, 0, m_TotalSize * sizeof(T));
		bufferManager.executeReads();
		m_GPUDataDirty = false;
	}

	template<typename T>
	void Tensor<T>::updateCPUData() const {
		Graphics::BufferManager& bufferManager = Core::Application::getApplication()->getRenderer()->getBufferManager();

		Graphics::MappedBuf readBuf = bufferManager.beginMappedRead(m_GPUDataHandle, m_TotalSize * sizeof(T));
		bufferManager.submitMappedRead(readBuf, 0, m_TotalSize * sizeof(T));
		bufferManager.executeReads();

		//Core::Logger::Warn("Updating CPU data from GPU");

		std::copy_n(static_cast<T*>(readBuf.mappedMemory), m_TotalSize, m_DataPtr);

		m_CPUDataDirty = false;
	}

	template<typename T>
	void Tensor<T>::createGPUData()
	{
		Graphics::BufferManager& bufferManager = Core::Application::getApplication()->getRenderer()->getBufferManager();

		if (m_GPUDataHandle == 0)
			m_GPUDataHandle = bufferManager.createBuffer(false, m_TotalSize * sizeof(T));

	}

	template<typename T>
	T Tensor<T>::getMinValue()
	{
		if (m_MinValue != std::numeric_limits<float>::max())
			return m_MinValue;

		for (size_t i = 0; i < m_TotalSize; i++)
		{
			if (i == 0 || m_DataPtr[i] < m_MinValue)
				m_MinValue = m_DataPtr[i];
		}

		return m_MinValue;
	}

	template<typename T>
	T Tensor<T>::getMaxValue()
	{
		if (m_MaxValue != std::numeric_limits<float>::lowest())
			return m_MaxValue;

		for (size_t i = 0; i < m_TotalSize; i++)
		{
			if (i == 0 || m_DataPtr[i] > m_MaxValue)
				m_MaxValue = m_DataPtr[i];
		}

		return m_MaxValue;
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
