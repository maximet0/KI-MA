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

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = renderer->getNextDSVDescriptorHandle(m_DSVDescriptorIndex);
		device->CreateDepthStencilView(m_DepthStencil.Get(), nullptr, dsvHandle);


		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		device->CreateShaderResourceView(m_RenderTarget.Get(), &srvDesc, renderer->getTextureManager().getNextSRVDescriptorHandle(m_SRVDescriptorIndex));
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
		device->CreateDepthStencilView(m_DepthStencil.Get(), nullptr, renderer->getDSVDescriptorHandle(m_DSVDescriptorIndex));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		device->CreateShaderResourceView(m_RenderTarget.Get(), &srvDesc, renderer->getTextureManager().getSRVDescriptorHandle(m_SRVDescriptorIndex));
	}

	void RenderTarget::copyToCpuBuffer(void* buffer, size_t bufferSize)
	{
		auto renderer = Core::Application::getApplication()->getRenderer();
		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();

		const D3D12_RESOURCE_DESC desc = m_RenderTarget->GetDesc();
		uint32_t width = static_cast<uint32_t>(desc.Width);
		uint32_t height = desc.Height;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		UINT numRows = 0;
		UINT64 rowSizeBytes = 0;
		UINT64 uploadSize = 0;
		device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &numRows, &rowSizeBytes, &uploadSize);

		auto bufManager = &renderer->getBufferManager();

		BufferHandle readback = bufManager->createBuffer(true, uploadSize);
	

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.pResource = m_RenderTarget.Get();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcLocation.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		dstLocation.pResource = bufManager->getBuffer(readback).Get();
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dstLocation.PlacedFootprint = footprint;

		renderer->transition(m_RenderTarget.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
		renderer->transition(bufManager->getBuffer(readback).Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		renderer->getCmdList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

		renderer->transition(m_RenderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		renderer->transition(bufManager->getBuffer(readback).Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);


		MappedBuf mappedRead = bufManager->beginMappedRead(readback, bufferSize);

		for (uint32_t row = 0; row < numRows; row++) {
			bufManager->submitMappedRead(mappedRead, footprint.Offset + row * footprint.Footprint.RowPitch, width * 4);
		}

		bufManager->executeReads();
		memcpy(buffer, mappedRead.mappedMemory, bufferSize);
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

		resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE depthClearValue = {};
		depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthClearValue.DepthStencil.Depth = 1.0f;
		depthClearValue.DepthStencil.Stencil = 0;

		device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&m_DepthStencil));
	}
}


