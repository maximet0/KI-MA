#pragma once
#include <vector>
#include <assert.h>
#include <array>
#include <limits>

#include "Core/Logger.h"

#include "Graphics/BufferManager.h"

namespace AI {

	template<typename T>
	class TensorView;

	template<typename T>
	class Tensor {
	public:
		Tensor();
		Tensor(const std::vector<size_t>& shape);
		Tensor(const Tensor& other) : Tensor(other.m_Shape) {
			if (other.isCPUDataDirty())
				other.updateCPUData();

			if (m_TotalSize != 0)
				std::copy_n(other.m_DataPtr, m_TotalSize, m_DataPtr);
		}
		Tensor(Tensor&& other) noexcept {
			swap(other);
		}

		~Tensor();

		size_t getTotalSize() const { return m_TotalSize; }

		TensorView<T> view() {
			return TensorView<T>(*this, 0, m_TotalSize, m_Shape);
		}

		template<typename... Indices>
		TensorView<T> view(std::vector<size_t> shape, Indices... indices) {
			size_t index = getIndex(indices...);
			size_t size = 1;
			for(auto& s : shape)
				size *= s;
			return TensorView<T>(*this, index, size, shape);
		}

		// eindimensionaler Zugriff
		T& operator[](size_t index) {
			if (m_CPUDataDirty)
				updateCPUData();

			m_GPUDataDirty = true;

			return m_DataPtr[index];
		}

		const T& operator[](size_t index) const {
			if(m_CPUDataDirty)
				updateCPUData();

			return m_DataPtr[index];
		}

		// dimensionaler Zugriff
		template<typename... Indices>
		T& operator()(Indices... indices) {
			if (m_CPUDataDirty)
				updateCPUData();

			size_t index = getIndex(indices...);
			m_GPUDataDirty = true;
			return m_DataPtr[index];
		}

		template<typename... Indices>
		const T& operator()(Indices... indices) const {
			if (m_CPUDataDirty)
				updateCPUData();

			size_t index = getIndex(indices...);
			return m_DataPtr[index];
		}

		Tensor& operator=(std::initializer_list<T> values) {
			if (values.size() != m_TotalSize)
				Core::Logger::Fatal("Initializer list size does not match tensor size.");

			std::copy(values.begin(), values.end(), m_DataPtr);
			m_GPUDataDirty = true;
			return *this;
		}
		
		Tensor& operator=(Tensor&& other) noexcept {
			if (this != &other) {
				swap(other);
			}
			return *this;
		}

		Tensor& operator=(const Tensor& other) {
			if (this != &other) {
				Tensor temp(other);
				swap(temp);
			}
			return *this;
		}

		void swap(Tensor& other) noexcept {
			std::swap(m_DataPtr, other.m_DataPtr);
			std::swap(m_TotalSize, other.m_TotalSize);
			std::swap(m_Shape, other.m_Shape);
			std::swap(m_Strides, other.m_Strides);
			std::swap(m_CachedPackedDataPtr, other.m_CachedPackedDataPtr);
			std::swap(m_CachedPackedDataValid, other.m_CachedPackedDataValid);
			std::swap(m_MinValue, other.m_MinValue);
			std::swap(m_MaxValue, other.m_MaxValue);
			std::swap(m_GPUDataDirty, other.m_GPUDataDirty);
			std::swap(m_CPUDataDirty, other.m_CPUDataDirty);
			std::swap(m_GPUDataHandle, other.m_GPUDataHandle);
		}

		void copyTo(Tensor& target);

		Tensor copy() const;

		void genPackedData(uint32_t NSize, uint32_t KSize);
		void clearCachedData();

		bool isCPUDataDirty() const { return m_CPUDataDirty; }
		bool isGPUDataDirty() const { return m_GPUDataDirty; }

		bool gpuDataExists() const { return m_GPUDataHandle != 0; }

		void markGPUWritten() { m_CPUDataDirty = true; m_GPUDataDirty = false; }
		void markCPUWritten() { m_CPUDataDirty = false; m_GPUDataDirty = true; }

		void updateGPUData();
		void updateCPUData() const;

		void createGPUData();

		void setShape(const std::vector<size_t>& shape);

		Graphics::BufferHandle getGPUDataHandle() const { return m_GPUDataHandle; }

		T* getDataPtr() { return m_DataPtr; }
		T* getCachedPackedDataPtr() { return m_CachedPackedDataValid ? m_CachedPackedDataPtr : nullptr; }

		const T* getDataPtr() const { return m_DataPtr; }
		const T* getCachedPackedDataPtr() const { return m_CachedPackedDataPtr; }

		const std::vector<size_t>& getShape() const { return m_Shape; }
		const std::vector<size_t>& getStrides() const { return m_Strides; }

		T getMinValue();
		T getMaxValue();

	private:


		// Berechnet den index für den dimensionalen Zugriff
		template<typename... Indices>
		size_t getIndex(Indices... indices) {
			std::array<size_t, sizeof...(Indices)> idx = { static_cast<size_t>(indices)... };
			assert(idx.size() == m_Shape.size());

			size_t index = 0;
			for (size_t i = 0; i < idx.size(); i++)
				index += idx[i] * m_Strides[i];

			return index;	
		}

		T* m_DataPtr = nullptr;
		size_t m_TotalSize = 0;

		std::vector<size_t> m_Shape ;
		std::vector<size_t> m_Strides;

		T* m_CachedPackedDataPtr = nullptr;
		bool m_CachedPackedDataValid = false;


		T m_MinValue = std::numeric_limits<float>::max();
		T m_MaxValue = std::numeric_limits<float>::lowest();

		bool m_GPUDataDirty = true;
		mutable bool m_CPUDataDirty = false;
		Graphics::BufferHandle m_GPUDataHandle = 0;

	};

	template<typename T>
	class TensorView {
	public:
		TensorView(Tensor<T>& base, size_t offset, size_t viewSize, const std::vector<size_t>& shape);
		~TensorView();

		// eindimensionaler Zugriff
		T& operator[](size_t index) {
			return (*m_BaseTensor)[index + m_Offset];
		}

		const T& operator[](size_t index) const {
			return (*m_BaseTensor)[index + m_Offset];
		}

		// dimensionaler Zugriff
		template<typename... Indices>
		T& operator()(Indices... indices) {
			size_t index = getIndex(indices...);
			return (*m_BaseTensor)[index + m_Offset];
		}

		template<typename... Indices>
		const T& operator()(Indices... indices) const {
			size_t index = getIndex(indices...);
			return (*m_BaseTensor)[index + m_Offset];
		}

		Tensor<T> viewCopy() const;

		size_t getTotalSize() const { return m_ViewSize; }

		const std::vector<size_t>& getShape() const { return m_ViewShape; }
		const std::vector<size_t>& getStrides() const { return m_ViewStrides; }

		const T* getDataPtr() const { return m_BaseTensor->getDataPtr() + m_Offset; }
		T* getDataPtr() { return m_BaseTensor->getDataPtr() + m_Offset; }

	private:
		// Berechnet den index für den dimensionalen Zugriff
		template<typename... Indices>
		size_t getIndex(Indices... indices) {
			std::array<size_t, sizeof...(Indices)> idx = { static_cast<size_t>(indices)... };
			assert(idx.size() == m_ViewShape.size());

			size_t index = 0;
			for (size_t i = 0; i < idx.size(); i++)
				index += idx[i] * m_ViewStrides[i];

			return index;
		}

		size_t m_Offset;
		size_t m_ViewSize;
		Tensor<T>* m_BaseTensor;
		std::vector<size_t> m_ViewShape;
		std::vector<size_t> m_ViewStrides;
	};
}