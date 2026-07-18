#pragma once
#include "Core/Window.h"
#include "Renderer.h"


#include <dxgi1_6.h>
#include <d3d12.h>
#include <stdint.h>

namespace Graphics {
	class Swapchain {
	public:
		Swapchain(Core::Window* window);
		~Swapchain();

		void Present();

		void resize(DirectX::XMINT2 size);

		uint32_t getBackBufferIndex();
		ID3D12Resource* getCurrentBackBuffer();

		D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV();

	private:
		IDXGISwapChain4* m_Swapchain;
		ID3D12DescriptorHeap* m_RTVHeap;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_BackBuffers[frameCount];

		uint32_t m_RTVDescriptorSize;

	};
}