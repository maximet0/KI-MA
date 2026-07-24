#include "Swapchain.h"

#include "Core/Application.h"

#include <stdexcept>

namespace Graphics {

	Swapchain::Swapchain(Core::Window* window)
	{ 
		Core::Application* app = Core::Application::getApplication();
		auto factory = app->getGraphicsContext()->getFactory();
		auto device = app->getGraphicsContext()->getDevice();
		auto renderer = app->getRenderer();

		// Swapchain erstellen
		HRESULT res = {};
		DXGI_SWAP_CHAIN_DESC1 swapDesc = { };
		swapDesc.BufferCount = frameCount;
		swapDesc.Width = static_cast<uint32_t>(window->getSize().x);
		swapDesc.Height = static_cast<uint32_t>(window->getSize().y);
		swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapDesc.SampleDesc.Count = 1;
		swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		res = factory->CreateSwapChainForHwnd(app->getGraphicsContext()->getQueue().Get(), window->getHandle(), &swapDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&m_Swapchain));
		if (FAILED(res)) throw std::runtime_error("");

		// Swapchain-Descriptor-Heap erstellen

		for (UINT i = 0; i < frameCount; i++)
		{
			m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i]));
			device->CreateRenderTargetView(m_BackBuffers[i].Get(), nullptr, renderer->getNextRTVDescriptorHandle(m_RTVDescriptorIndices[i]));
		}

	}

	Swapchain::~Swapchain()
	{
		for (UINT i = 0; i < frameCount; i++)
		{
			m_BackBuffers[i]->Release();
		}
		m_Swapchain->Release();
	}

	void Swapchain::Present()
	{
		// Den Backbuffer präsentieren
		m_Swapchain->Present(0, 0);
	}

	void Swapchain::resize(DirectX::XMINT2 size)
	{
		Core::Application* app = Core::Application::getApplication();
		auto factory = app->getGraphicsContext()->getFactory();
		auto device = app->getGraphicsContext()->getDevice();

		// Swapchain Grösse anpassen
		for (UINT i = 0; i < frameCount; i++)
		{
			m_BackBuffers[i].Reset();
		}
		
		m_Swapchain->ResizeBuffers(frameCount, size.x, size.y, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

		for (UINT i = 0; i < frameCount; i++)
		{
			m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i]));
			device->CreateRenderTargetView(m_BackBuffers[i].Get(), nullptr, app->getRenderer()->getNextRTVDescriptorHandle(m_RTVDescriptorIndices[i]));
		}
	}

	uint32_t Swapchain::getBackBufferIndex() {
		return m_Swapchain->GetCurrentBackBufferIndex();
	}

	ID3D12Resource* Swapchain::getCurrentBackBuffer()
	{
		UINT frame = m_Swapchain->GetCurrentBackBufferIndex();
		return m_BackBuffers[frame].Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Swapchain::getCurrentRTV()
	{
		auto renderer = Core::Application::getApplication()->getRenderer();
		return renderer->getRTVDescriptorHandle(m_RTVDescriptorIndices[getBackBufferIndex()]);
	}

}
