#include "ComputeContext.h"

namespace AI::GPU {
	ComputeContext::ComputeContext(Graphics::GraphicsContext* graphicsContext)
		: m_GraphicsContext(graphicsContext) {

		Microsoft::WRL::ComPtr<ID3D12Device14> device = m_GraphicsContext->getDevice();

		// Command Allocator erstellen.
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_CmdAllocator));

		// Die Command List erstellen
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_CmdAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CmdList));
		m_CmdList->Close();

		// Fence erstellen
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
		m_FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	}

	ComputeContext::~ComputeContext() {
	
	}

	void ComputeContext::submit() {
	
	}

	void ComputeContext::synchronize() {
		
	}

}

