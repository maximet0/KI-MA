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


		m_RectDataBuf = createBuffer(sizeof(RectData) * maxRects, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_RectDataBuf->Map(0, nullptr, reinterpret_cast<void**>(&m_RectDataPtr));

		m_LineDataBuf = createBuffer(sizeof(LineData) * maxLines, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_LineDataBuf->Map(0, nullptr, reinterpret_cast<void**>(&m_LineDataPtr));

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
			// TODO: Implement Allocator
		};
		ImGui_ImplDX12_Init(&init_info);
	}

	void Renderer::beginFrame() {
		Core::Application* app = Core::Application::getApplication();
		uint32_t frameIndex = app->getSwapchain()->getBackBufferIndex();
		Graphics::GraphicsContext* context = app->getGraphicsContext();

		// CmdAllocator und CmdList zurücksetzen
		m_CmdAllocators[frameIndex]->Reset();
		m_CmdList->Reset(m_CmdAllocators[frameIndex], 0);

		// Descriptor Heap setzen
		m_CmdList->SetDescriptorHeaps(1, &m_TextureManager.getSRVDescriptorHeap());

		m_RectCount = 0;
		m_CurrentRectOffset = 0;

		m_LineCount = 0;
		m_CurrentLineOffset = 0;
	}


	ID3D12Resource* Renderer::createBuffer(size_t size, D3D12_HEAP_TYPE type, D3D12_RESOURCE_STATES initState) {
		Core::Application* app = Core::Application::getApplication();
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = type;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = size;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc = { 1, 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ID3D12Resource* buffer = nullptr;

		app->getGraphicsContext()->getDevice()->CreateCommittedResource(
			&heapProperties, D3D12_HEAP_FLAG_NONE, 
			&resourceDesc, initState, 
			nullptr, IID_PPV_ARGS(&buffer)
		);

		return buffer;
	}


	void Renderer::submitRect(DirectX::XMFLOAT2 position, DirectX::XMFLOAT2 size, uint32_t textureHandle)
	{
		m_RectDataPtr[m_RectCount].pos = { position.x + size.x / 2, position.y + size.y / 2 };
		m_RectDataPtr[m_RectCount].size = size;
		m_RectDataPtr[m_RectCount].textureID = textureHandle;

		m_RectCount++;
		m_CurrentRectCount++;
	}

	void Renderer::submitLine(DirectX::XMFLOAT2 begin, DirectX::XMFLOAT2 end, uint32_t color)
	{
		m_LineDataPtr[m_LineCount].begin = begin;
		m_LineDataPtr[m_LineCount].beginColor = color;
		m_LineDataPtr[m_LineCount].end = end;
		m_LineDataPtr[m_LineCount].endColor = color;

		m_LineCount++;
		m_CurrentLineCount++;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuf;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuf;

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
			
			vertexBuf = createBuffer(sizeof(vertices), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
			indexBuf = createBuffer(sizeof(indices), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

			void* mappedData;
			vertexBuf->Map(0, nullptr, &mappedData);
			memcpy(mappedData, vertices, sizeof(vertices));
			vertexBuf->Unmap(0, nullptr);

			indexBuf->Map(0, nullptr, &mappedData);
			memcpy(mappedData, indices, sizeof(indices));
			indexBuf->Unmap(0, nullptr);
		}

		m_DefaultPipeline.usePipeline(m_CmdList);

		m_CmdList->SetGraphicsRootConstantBufferView(0, m_CurrentVPBuf->GetGPUVirtualAddress());
		m_CmdList->SetGraphicsRootShaderResourceView(1, m_RectDataBuf->GetGPUVirtualAddress());


		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = vertexBuf->GetGPUVirtualAddress();
		vertexBufferView.SizeInBytes = sizeof(float) * 16;
		vertexBufferView.StrideInBytes = sizeof(float) * 4;

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = indexBuf->GetGPUVirtualAddress();
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
		m_LinePipeline.usePipeline(m_CmdList);

		m_CmdList->SetGraphicsRootConstantBufferView(0, m_CurrentVPBuf->GetGPUVirtualAddress());

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = m_LineDataBuf->GetGPUVirtualAddress();
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

		m_CmdList->Close();

		ID3D12CommandList* lists[] = {
			m_CmdList
		};

		// Die Command List an die GPU senden
		app->getGraphicsContext()->getQueue()->ExecuteCommandLists(1, lists);
		
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
		
	}

	void Renderer::beginRenderTarget(RenderTarget* renderTarget, DirectX::XMFLOAT2 cameraPosition, float cameraZoom)
	{
		Core::Application* app = Core::Application::getApplication();
		uint32_t frameIndex = app->getSwapchain()->getBackBufferIndex();

		transition(renderTarget->getRenderTarget(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		auto rtv = getRTVDescriptorHandle(renderTarget->getRTVDescriptorIndex());

		m_CmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		DirectX::XMFLOAT4 clearColor = renderTarget->getClearColor();
		float color[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
		m_CmdList->ClearRenderTargetView(rtv, color, 0, nullptr);

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

		m_CurrentVPBuf = createBuffer(sizeof(DirectX::XMMATRIX), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

		DirectX::XMMATRIX cameraView = DirectX::XMMatrixTranslation(-cameraPosition.x, -cameraPosition.y, 0.0f);
		cameraView = cameraView * DirectX::XMMatrixScaling(cameraZoom, cameraZoom, 1.0f) ;
		DirectX::XMMATRIX cameraProj = DirectX::XMMatrixOrthographicOffCenterLH(-(float)renderTarget->getSize().x * 0.5f, (float)renderTarget->getSize().x * 0.5f, (float)renderTarget->getSize().y * 0.5f, -(float)renderTarget->getSize().y * 0.5f, 0.0f, 1.0f);
		DirectX::XMMATRIX cameraVP = cameraView * cameraProj;

		void* mappedData;
		m_CurrentVPBuf->Map(0, nullptr, &mappedData);
		memcpy(mappedData, &cameraVP, sizeof(DirectX::XMMATRIX));
		m_CurrentVPBuf->Unmap(0, nullptr);
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

}

