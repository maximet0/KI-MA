#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <stdint.h>


namespace Graphics {

	constexpr uint32_t frameCount = 2;

	class Renderer {
	public:
		Renderer();
		~Renderer();
	
		void initImGui();
		void renderImGui();

		void beginFrame();
		Microsoft::WRL::ComPtr<ID3D12Resource> createBuffer(size_t size, D3D12_HEAP_TYPE type, D3D12_RESOURCE_STATES initState);
		void drawRectangle();
		void endFrame();

		void waitForGPU();


	private:	

		void transition(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

		ID3D12GraphicsCommandList* m_CmdList;
		ID3D12CommandAllocator* m_CmdAllocators[frameCount];
		
		ID3D12Fence* m_Fence;
		uint64_t m_FenceValues[frameCount] = {};
		HANDLE m_FenceEvent;

		ID3D12DescriptorHeap* m_SRVHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHeapCPUCurrent;
		D3D12_GPU_DESCRIPTOR_HANDLE m_SRVHeapGPUCurrent;
		uint32_t m_SRVHeapHandleIncrement;
	};

}