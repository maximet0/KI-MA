#pragma once

#include "Graphics/GraphicsContext.h"

namespace AI::GPU {

	class ComputeContext {
		ComputeContext(Graphics::GraphicsContext* graphicsContext);
		~ComputeContext();

		void submit();
		void synchronize();


	private:
		Graphics::GraphicsContext* m_GraphicsContext;

		bool cmdListOpen = false;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CmdList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CmdAllocator;

		Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence = nullptr;
		uint64_t m_FenceValues = {};
		HANDLE m_FenceEvent = nullptr;



	};


}