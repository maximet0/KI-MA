#pragma once
#include <vector>

namespace AI {

	template<typename T>
	class TensorView;

	template<typename T>
	class Tensor {
	public:
		Tensor(const std::vector<size_t>& shape);
		~Tensor();

		size_t getTotalSize() const { return m_TotalSize; }

		TensorView<T> view() {
			return TensorView<T>(*this, 0, m_TotalSize, m_Shape);
		}

		// eindimensionaler Zugriff
		T& operator[](size_t index) {
			return m_DataPtr[index];
		}

		const T& operator[](size_t index) const {
			return m_DataPtr[index];
		}

		// dimensionaler Zugriff
		template<typename... Indices>
		T& operator()(Indices... indices) {
			size_t index = getIndex(indices...);
			return m_DataPtr[index];
		}

		template<typename... Indices>
		const T& operator()(Indices... indices) const {
			size_t index = getIndex(indices...);
			return m_DataPtr[index];
		}

		const T* getDataPtr() const { return m_DataPtr; }

		const std::vector<size_t>& getShape() const { return m_Shape; }
		const std::vector<size_t>& getStrides() const { return m_Strides; }

	private:
		void setShape(const std::vector<size_t>& shape);

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

		T* m_DataPtr;
		size_t m_TotalSize;

		std::vector<size_t> m_Shape;
		std::vector<size_t> m_Strides;
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

		size_t getTotalSize() const { return m_BaseTensor->getTotalSize(); }

		const std::vector<size_t>& getShape() const { return m_ViewShape; }
		const std::vector<size_t>& getStrides() const { return m_ViewStrides; }

		const T* getDataPtr() const { return m_BaseTensor->getDataPtr() + m_Offset; }

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