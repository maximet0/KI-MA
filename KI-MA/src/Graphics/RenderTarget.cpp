#include "RenderTarget.h"

#include "Core/Application.h"

namespace Graphics {
	Graphics::RenderTarget::RenderTarget(DirectX::XMINT2 size, DirectX::XMFLOAT4 clearColor)
		: m_ClearColor(clearColor)
	{
		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();
		auto renderer = Core::Application::getApplication()->getRenderer();

		m_Size = size;
		createRenderTarget(m_ClearColor);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = renderer->getNextRTVDescriptorHandle(m_RTVDescriptorIndex);
		device->CreateRenderTargetView(m_RenderTarget.Get(), nullptr, rtvHandle);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		device->CreateShaderResourceView(m_RenderTarget.Get(), &srvDesc, renderer->getNextSRVDescriptorHandle(m_SRVDescriptorIndex));
	}

	Graphics::RenderTarget::~RenderTarget()
	{
	
	}

	void Graphics::RenderTarget::resize(DirectX::XMINT2 size)
	{
		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();
		auto renderer = Core::Application::getApplication()->getRenderer();
		m_Size = size;
		createRenderTarget(m_ClearColor);
		device->CreateRenderTargetView(m_RenderTarget.Get(), nullptr, renderer->getRTVDescriptorHandle(m_RTVDescriptorIndex));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		device->CreateShaderResourceView(m_RenderTarget.Get(), &srvDesc, renderer->getSRVDescriptorHandle(m_SRVDescriptorIndex));
	}

	void RenderTarget::createRenderTarget(DirectX::XMFLOAT4 clearColor) {
		Core::Application* app = Core::Application::getApplication();
		auto device = app->getGraphicsContext()->getDevice();

		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = m_Size.x;
		resourceDesc.Height = m_Size.y;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resourceDesc.SampleDesc = { 1, 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		clearValue.Color[0] = clearColor.x;
		clearValue.Color[1] = clearColor.y;
		clearValue.Color[2] = clearColor.z;
		clearValue.Color[3] = clearColor.w;

		device->CreateCommittedResource(
			&heapProperties, D3D12_HEAP_FLAG_NONE,
			&resourceDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue, IID_PPV_ARGS(&m_RenderTarget)
		);


	}
}


