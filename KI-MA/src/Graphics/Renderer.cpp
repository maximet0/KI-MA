#include "Renderer.h"

#include "Core/Logger.h"
#include "Core/Application.h"
#include "external/ImGui/backends/imgui_impl_dx12.h"
#include <fstream>

namespace Graphics {


	Renderer::Renderer() {
		Core::Application* app = Core::Application::getApplication();
		auto device = app->getGraphicsContext()->getDevice();

		// Für jeden Frame einen Command Allocator erstellen.
		for (int i = 0; i < frameCount; i++) {
			device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CmdAllocators[i]));
		}

		// Die Command List erstellen
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CmdAllocators[0], nullptr, IID_PPV_ARGS(&m_CmdList));
		m_CmdList->Close();

		// Fence erstellen
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
		m_FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);


		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.NumDescriptors = 1024;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap));
		m_RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_RTVHeapCPUStart = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors = 1024;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

		device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap));
		m_DSVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_DSVHeapCPUStart = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();


		m_DefaultPipeline.setShaders("VertexShader.cso", "PixelShader.cso");
		m_DefaultPipeline.addInputElement({ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		m_DefaultPipeline.addInputElement({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParam.Descriptor.ShaderRegister = 0;
		rootParam.Descriptor.RegisterSpace = 0;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		m_DefaultPipeline.addRootParameter(rootParam);
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParam.Descriptor.ShaderRegister = 0;
		rootParam.Descriptor.RegisterSpace = 0;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		m_DefaultPipeline.addRootParameter(rootParam);

		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.ShaderRegister = 0;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		m_DefaultPipeline.addStaticSampler(samplerDesc);

		PipelineSettings settings;
		m_DefaultPipeline.recreatePipelineState(device.Get(), settings);

		m_LinePipeline.setShaders("LineVertexShader.cso", "LinePixelShader.cso");
		m_LinePipeline.addInputElement({ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		m_LinePipeline.addInputElement({ "COLOR", 0, DXGI_FORMAT_R32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParam.Descriptor.ShaderRegister = 0;
		rootParam.Descriptor.RegisterSpace = 0;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		m_LinePipeline.addRootParameter(rootParam);

		settings.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		m_LinePipeline.recreatePipelineState(device.Get(), settings);

		m_RectDataBuf = m_BufferManager.createBuffer(false, sizeof(RectData) * maxRects);
		m_LineDataBuf = m_BufferManager.createBuffer(false, sizeof(LineData) * maxLines);
	}

	Renderer::~Renderer() {
		CloseHandle(m_FenceEvent);
	}

	void Renderer::initImGui()
	{
		// ImGui für DirectX 12 initialisieren
		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = Core::Application::getApplication()->getGraphicsContext()->getDevice().Get();
		init_info.CommandQueue = Core::Application::getApplication()->getGraphicsContext()->getQueue().Get();
		init_info.NumFramesInFlight = frameCount;
		init_info.RTVFormat = Core::Application::getApplication()->getSwapchain()->getCurrentBackBuffer()->GetDesc().Format;
		init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
		init_info.SrvDescriptorHeap = m_TextureManager.getSRVDescriptorHeap();

		init_info.SrvDescriptorAllocFn = [&](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle) {
			uint32_t index = 0;
			out_cpu_desc_handle->ptr = m_TextureManager.getNextSRVDescriptorHandle(index).ptr;
			out_gpu_desc_handle->ptr = m_TextureManager.getSRVGPUDescriptorHandle(index).ptr;
		};

		init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle) {
			// TODO: Implement Free
		};
		ImGui_ImplDX12_Init(&init_info);
	}

	void Renderer::beginFrame() {
		Core::Application* app = Core::Application::getApplication();
		uint32_t frameIndex = app->getSwapchain()->getBackBufferIndex();
		Graphics::GraphicsContext* context = app->getGraphicsContext();

		// CmdAllocator und CmdList zurücksetzen
		beginCmdList(frameIndex);
		
		// Descriptor Heap setzen
		m_CmdList->SetDescriptorHeaps(1, &m_TextureManager.getSRVDescriptorHeap());

		m_RectCount = 0;
		m_CurrentRectOffset = 0;

		m_LineCount = 0;
		m_CurrentLineOffset = 0;

		m_RectMappedMem = m_BufferManager.beginMappedWrite(m_RectDataBuf, sizeof(RectData) * maxRects);
		m_LineMappedMem = m_BufferManager.beginMappedWrite(m_LineDataBuf, sizeof(LineData) * maxLines);
	}

	void Renderer::submitRect(DirectX::XMFLOAT2 position, float zLayer, DirectX::XMFLOAT2 size, uint32_t textureHandle, bool repeatTexture)
	{
		if (m_RectCount >= maxRects)
			return;

		DirectX::XMFLOAT2 texSize = m_TextureManager.getTextureSize(textureHandle);

		RectData& rectData = ((RectData*)m_RectMappedMem.mappedMemory)[m_RectCount];
		rectData.pos = { position.x + size.x / 2, position.y + size.y / 2 };
		rectData.size = size;
		rectData.textureScale = repeatTexture ? DirectX::XMFLOAT2{ size.x / texSize.x, size.y / texSize.y } : DirectX::XMFLOAT2{ 1.0f, 1.0f };
		rectData.zLayer = zLayer;
		rectData.textureID = textureHandle;

		m_RectCount++;
		m_CurrentRectCount++;
	}

	void Renderer::submitLine(DirectX::XMFLOAT2 begin, DirectX::XMFLOAT2 end, uint32_t color)
	{
		if (m_LineCount >= maxLines)
			return;

		LineData& lineData = ((LineData*)m_LineMappedMem.mappedMemory)[m_LineCount];
		lineData.begin = begin;
		lineData.beginColor = color;
		lineData.end = end;
		lineData.endColor = color;

		m_LineCount++;
		m_CurrentLineCount++;
	}


	BufferHandle vertexBuf;
	BufferHandle indexBuf;

	void Renderer::drawRects() {
		Core::Application* app = Core::Application::getApplication();
		Graphics::GraphicsContext* context = app->getGraphicsContext();

		if (!vertexBuf) {
			float vertices[] = {
				-0.5f, -0.5f, 0.0f, 0.0f,
				 0.5f, -0.5f, 1.0f, 0.0f,
				 0.5f,  0.5f, 1.0f, 1.0f,
				-0.5f,  0.5f, 0.0f, 1.0f,
			};

			int indices[] = {
				0, 1, 2,
				2, 3, 0
			};
			
			vertexBuf = m_BufferManager.createBuffer(false, sizeof(vertices));
			indexBuf = m_BufferManager.createBuffer(false, sizeof(indices));
			
			m_BufferManager.write(vertexBuf, vertices, 0, sizeof(vertices));
			m_BufferManager.write(indexBuf, indices, 0, sizeof(indices));

			m_BufferManager.transitionState(vertexBuf, D3D12_RESOURCE_STATE_GENERIC_READ);
			m_BufferManager.transitionState(indexBuf, D3D12_RESOURCE_STATE_GENERIC_READ);
		}

		m_BufferManager.submitMappedWrite(m_RectMappedMem, 0, m_RectCount * sizeof(RectData));

		m_DefaultPipeline.usePipeline(m_CmdList);

		m_CmdList->SetGraphicsRootConstantBufferView(0, m_BufferManager.getBuffer(m_CurrentVPBuf)->GetGPUVirtualAddress());
		m_CmdList->SetGraphicsRootShaderResourceView(1, m_BufferManager.getBuffer(m_RectDataBuf)->GetGPUVirtualAddress());


		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = m_BufferManager.getBuffer(vertexBuf)->GetGPUVirtualAddress();
		vertexBufferView.SizeInBytes = sizeof(float) * 16;
		vertexBufferView.StrideInBytes = sizeof(float) * 4;

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = m_BufferManager.getBuffer(indexBuf)->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = sizeof(int) * 6;

		m_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_CmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		m_CmdList->IASetIndexBuffer(&indexBufferView);
		m_CmdList->DrawIndexedInstanced(6, m_CurrentRectCount, 0, 0, m_CurrentRectOffset);

		m_CurrentRectOffset = m_RectCount;
		m_CurrentRectCount = 0;
	}

	void Renderer::drawLines()
	{
		m_BufferManager.submitMappedWrite(m_LineMappedMem,0 , m_LineCount * sizeof(LineData));

		
		m_LinePipeline.usePipeline(m_CmdList);

		m_CmdList->SetGraphicsRootConstantBufferView(0, m_BufferManager.getBuffer(m_CurrentVPBuf)->GetGPUVirtualAddress());

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = m_BufferManager.getBuffer(m_LineDataBuf)->GetGPUVirtualAddress();
		vertexBufferView.SizeInBytes = sizeof(LineData) * m_LineCount;
		vertexBufferView.StrideInBytes = sizeof(LineData) / 2;

		m_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		m_CmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		m_CmdList->DrawInstanced(m_CurrentLineCount * 2, 1, m_CurrentLineOffset * 2, 0);
		
		m_CurrentLineOffset = m_LineCount;
		m_CurrentLineCount = 0;
	}

	uint32_t g_FenceValue = 0;

	void Renderer::endFrame() {
		Core::Application* app = Core::Application::getApplication();		
		uint32_t frameIndex = app->getSwapchain()->getBackBufferIndex();

		transition(app->getSwapchain()->getCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		auto rtv = app->getSwapchain()->getCurrentRTV();

		// Render Target setzen und den Backbuffer mit einer Farbe füllen
		m_CmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		const float color[4] =
		{
			0.22f,
			0.22f,
			0.22f,
			1.0f
		};

		m_CmdList->ClearRenderTargetView(rtv, color, 0, nullptr);

		// Viewport und Scissor Rect setzen
		D3D12_VIEWPORT viewport = {};
		viewport.Height = (float)app->getWindow()->getSize().y;
		viewport.Width = (float)app->getWindow()->getSize().x;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissorRect = {};
		scissorRect.right = app->getWindow()->getSize().x;
		scissorRect.bottom = app->getWindow()->getSize().y;
		scissorRect.left = 0;
		scissorRect.top = 0;

		m_CmdList->RSSetViewports(1, &viewport);
		m_CmdList->RSSetScissorRects(1, &scissorRect);
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CmdList);

		// Den Backbuffer in den Present State überführen
		transition(app->getSwapchain()->getCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		endCmdList();

		g_FenceValue++;
		app->getGraphicsContext()->getQueue()->Signal(m_Fence, g_FenceValue);
		m_FenceValues[frameIndex] = g_FenceValue;

		frameIndex = app->getSwapchain()->getBackBufferIndex();

		// Den Backbuffer präsentieren
		app->getSwapchain()->Present();

		if (m_Fence->GetCompletedValue() < m_FenceValues[frameIndex])
		{
			m_Fence->SetEventOnCompletion(m_FenceValues[frameIndex], m_FenceEvent);
			WaitForSingleObject(m_FenceEvent, INFINITE);
		}

		frameIndex = app->getSwapchain()->getBackBufferIndex();
		m_BufferManager.clearOldBuffers(frameIndex);
	}

	void Renderer::beginRenderTarget(RenderTarget* renderTarget, DirectX::XMFLOAT2 cameraPosition, float cameraZoom)
	{
		Core::Application* app = Core::Application::getApplication();
		uint32_t frameIndex = app->getSwapchain()->getBackBufferIndex();

		transition(renderTarget->getRenderTarget(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		auto rtv = getRTVDescriptorHandle(renderTarget->getRTVDescriptorIndex());
		auto dsv = getDSVDescriptorHandle(renderTarget->getDSVDescriptorIndex());

		m_CmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		DirectX::XMFLOAT4 clearColor = renderTarget->getClearColor();
		float color[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
		m_CmdList->ClearRenderTargetView(rtv, color, 0, nullptr);
		m_CmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		D3D12_VIEWPORT viewport = {};
		viewport.Height = (float)renderTarget->getSize().y;
		viewport.Width = (float)renderTarget->getSize().x;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissorRect = {};
		scissorRect.right = renderTarget->getSize().x;
		scissorRect.bottom = renderTarget->getSize().y;
		scissorRect.left = 0;
		scissorRect.top = 0;

		m_CmdList->RSSetViewports(1, &viewport);
		m_CmdList->RSSetScissorRects(1, &scissorRect);

		m_CurrentRenderTarget = renderTarget;

		m_CurrentVPBuf = m_BufferManager.createBuffer(true, sizeof(DirectX::XMMATRIX));

		DirectX::XMMATRIX cameraView = DirectX::XMMatrixTranslation(-cameraPosition.x, -cameraPosition.y, 0.0f);
		cameraView = cameraView * DirectX::XMMatrixScaling(cameraZoom, cameraZoom, 1.0f) ;
		DirectX::XMMATRIX cameraProj = DirectX::XMMatrixOrthographicOffCenterLH(-(float)renderTarget->getSize().x * 0.5f, (float)renderTarget->getSize().x * 0.5f, (float)renderTarget->getSize().y * 0.5f, -(float)renderTarget->getSize().y * 0.5f, 0.0f, 10.0f);
		DirectX::XMMATRIX cameraVP = cameraView * cameraProj;

		m_BufferManager.write(m_CurrentVPBuf, &cameraVP, 0, sizeof(DirectX::XMMATRIX));
	}

	void Renderer::endRenderTarget(RenderTarget* renderTarget)
	{
		transition(renderTarget->getRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void Renderer::waitForGPU()
	{
		// Warten, bis die GPU alle Befehle abgeschlossen hat
		Core::Application* app = Core::Application::getApplication();
		const UINT64 value = ++g_FenceValue;

		app->getGraphicsContext()->getQueue()->Signal(m_Fence, value);

		if (m_Fence->GetCompletedValue() < value)
		{
			m_Fence->SetEventOnCompletion(value, m_FenceEvent);
			WaitForSingleObject(m_FenceEvent, INFINITE);
		}
	}
		
	D3D12_CPU_DESCRIPTOR_HANDLE Renderer::getRTVDescriptorHandle(uint32_t index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_RTVHeapCPUStart;
		handle.ptr += m_RTVDescriptorSize * index;
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Renderer::getNextRTVDescriptorHandle(uint32_t& index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_RTVHeapCPUStart;
		handle.ptr += m_RTVDescriptorSize * m_RTVHeapCurrentIndex;
		index = m_RTVHeapCurrentIndex;
		m_RTVHeapCurrentIndex++;
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Renderer::getDSVDescriptorHandle(uint32_t index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DSVHeapCPUStart;
		handle.ptr += m_DSVDescriptorSize * index;
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Renderer::getNextDSVDescriptorHandle(uint32_t& index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DSVHeapCPUStart;
		handle.ptr += m_RTVDescriptorSize * m_DSVHeapCurrentIndex;
		index = m_DSVHeapCurrentIndex;
		m_DSVHeapCurrentIndex++;
		return handle;
	}

	void Renderer::transition(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
		D3D12_RESOURCE_BARRIER barrier	{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		barrier.Transition.pResource = res;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		// Die Resource Barrier an die Command List senden
		m_CmdList->ResourceBarrier(1, &barrier); 
	}

	void Renderer::beginCmdList(uint32_t frameIndex)
	{
		m_CmdAllocators[frameIndex]->Reset();
		m_CmdList->Reset(m_CmdAllocators[frameIndex], nullptr);
		cmdListOpen = true;
	}

	void Renderer::endCmdList()
	{
		cmdListOpen = false;
		m_CmdList->Close();

		ID3D12CommandList* lists[] = {
			m_CmdList
		};

		Core::Application* app = Core::Application::getApplication();
		app->getGraphicsContext()->getQueue()->ExecuteCommandLists(1, lists);
	}

}

