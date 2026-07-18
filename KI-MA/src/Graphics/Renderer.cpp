#include "Renderer.h"


#include "Core/Application.h"
#include "ImGui/backends/imgui_impl_dx12.h"

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

		// Descriptor Heap für Shader Resource Views erstellen
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1000;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap));
		m_SRVHeapCPUCurrent = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();
		m_SRVHeapGPUCurrent = m_SRVHeap->GetGPUDescriptorHandleForHeapStart();
		m_SRVHeapHandleIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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
		init_info.SrvDescriptorHeap = m_SRVHeap;

		init_info.SrvDescriptorAllocFn = [&](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle) {
			out_cpu_desc_handle->ptr = m_SRVHeapCPUCurrent.ptr;
			out_gpu_desc_handle->ptr = m_SRVHeapGPUCurrent.ptr;

			m_SRVHeapCPUCurrent.ptr += m_SRVHeapHandleIncrement;
			m_SRVHeapGPUCurrent.ptr += m_SRVHeapHandleIncrement;
		};

		init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle) {
			// TODO: Implement Allocator
		};
		ImGui_ImplDX12_Init(&init_info);
	}

	void Renderer::renderImGui()
	{
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CmdList);
	}

	//TODO: Klasse für das Erstellen.
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

	void Renderer::beginFrame() {
		Core::Application* app = Core::Application::getApplication();
		uint32_t frameIndex = app->getSwapchain()->getBackBufferIndex();
		Graphics::GraphicsContext* context = app->getGraphicsContext();

		// Pipeline State und Root Signature erstellen, wenn sie noch nicht erstellt wurden  
		// TODO: Eine Pipeline Klasse erstellen.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		static D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
		static D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
		static bool shadersLoaded = false;
		if (!shadersLoaded) {
			// Shader-Dateien laden
			std::fstream vertexShaderFile("VertexShader.cso", std::ios::in | std::ios::binary);
			if (!vertexShaderFile.is_open()) {
				throw std::runtime_error("Failed to open VertexShader.cso");
			}
			vertexShaderFile.seekg(0, std::ios::end);
			uint32_t vertexShaderSize = vertexShaderFile.tellg();
			char* vertexShaderData = new char[vertexShaderSize];
			vertexShaderFile.seekg(0, std::ios::beg);
			vertexShaderFile.read(vertexShaderData, vertexShaderSize);

			std::fstream pixelShaderFile("PixelShader.cso", std::ios::in | std::ios::binary);
			if (!pixelShaderFile.is_open()) {
				throw std::runtime_error("Failed to open PixelShader.cso");
			}
			pixelShaderFile.seekg(0, std::ios::end);
			uint32_t pixelShaderSize = pixelShaderFile.tellg();
			char* pixelShaderData = new char[pixelShaderSize];
			pixelShaderFile.seekg(0, std::ios::beg);
			pixelShaderFile.read(pixelShaderData, pixelShaderSize);

			vertexShaderFile.close();
			pixelShaderFile.close();

			vertexShaderBytecode.pShaderBytecode = vertexShaderData;
			vertexShaderBytecode.BytecodeLength = vertexShaderSize;

			pixelShaderBytecode.pShaderBytecode = pixelShaderData;
			pixelShaderBytecode.BytecodeLength = pixelShaderSize;

			D3D12_ROOT_PARAMETER rootParameters[2] = {};
			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[0].Descriptor.ShaderRegister = 0;
			rootParameters[0].Descriptor.RegisterSpace = 0;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			rootParameters[1].Descriptor.ShaderRegister = 0;
			rootParameters[1].Descriptor.RegisterSpace = 0;
			rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
			rootSignatureDesc.NumParameters = 2;
			rootSignatureDesc.pParameters = rootParameters;
			rootSignatureDesc.NumStaticSamplers = 0;
			rootSignatureDesc.pStaticSamplers = nullptr;
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, nullptr);


			context->getDevice()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = rootSignature.Get();
			psoDesc.VS = vertexShaderBytecode;
			psoDesc.PS = pixelShaderBytecode;

			psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
			psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			psoDesc.RasterizerState.DepthClipEnable = TRUE;
			psoDesc.RasterizerState.MultisampleEnable = FALSE;
			psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;

			psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;

			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.SampleDesc.Count = 1;
			psoDesc.SampleDesc.Quality = 0;


			context->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

			shadersLoaded = true;
		}


		// CmdAllocator und CmdList zurücksetzen
		m_CmdAllocators[frameIndex]->Reset();
		m_CmdList->Reset(m_CmdAllocators[frameIndex], 0);

		// Den Backbuffer in den Render Target State überführen
		transition(app->getSwapchain()->getCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		auto rtv = app->getSwapchain()->getCurrentRTV();

		// Render Target setzen und den Backbuffer mit einer Farbe füllen
		m_CmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		const float color[4] =
		{
			0.2f,
			0.3f,
			0.7f,
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

		// Descriptor Heap setzen
		m_CmdList->SetDescriptorHeaps(1, &m_SRVHeap);

	}

	Microsoft::WRL::ComPtr<ID3D12Resource> Renderer::createBuffer(size_t size, D3D12_HEAP_TYPE type, D3D12_RESOURCE_STATES initState) {
		Core::Application* app = Core::Application::getApplication();
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

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

		Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

		app->getGraphicsContext()->getDevice()->CreateCommittedResource(
			&heapProperties, D3D12_HEAP_FLAG_NONE, 
			&resourceDesc, initState, 
			nullptr, IID_PPV_ARGS(&buffer)
		);
		return buffer;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuf;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuf;

	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuf;
	Microsoft::WRL::ComPtr<ID3D12Resource> rectBuf;

	void* constBufPtr;
	void* rectBufPtr;

	void Renderer::drawRectangle() {
		// Rechteck zeichnen

		Core::Application* app = Core::Application::getApplication();
		Graphics::GraphicsContext* context = app->getGraphicsContext();

		if (!vertexBuf) {
			float vertices[] = {
			-0.5f, -0.5f,
			 0.5f, -0.5f,
			 0.5f,  0.5f,
			-0.5f,  0.5f,
			};

			int indices[] = {
				0, 1, 2,
				2, 3, 0
			};

			vertexBuf = createBuffer(sizeof(vertices), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
			indexBuf = createBuffer(sizeof(indices), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

			constantBuf = createBuffer(sizeof(DirectX::XMMATRIX), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
			rectBuf = createBuffer(sizeof(float) * 4, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

			void* mappedData;
			vertexBuf->Map(0, nullptr, &mappedData);
			memcpy(mappedData, vertices, sizeof(vertices));
			vertexBuf->Unmap(0, nullptr);

			indexBuf->Map(0, nullptr, &mappedData);
			memcpy(mappedData, indices, sizeof(indices));
			indexBuf->Unmap(0, nullptr);

			constantBuf->Map(0, nullptr, &constBufPtr);
			rectBuf->Map(0, nullptr, &rectBufPtr);
		}

		struct Rect
		{
			DirectX::XMFLOAT2 pos;
			DirectX::XMFLOAT2 size;
		};

		DirectX::XMMATRIX cameraView = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX cameraProjection = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, (float)app->getWindow()->getSize().x, (float)app->getWindow()->getSize().y, 0.0f, 0.0f, 1.0f);

		DirectX::XMMATRIX mvp = cameraView * cameraProjection;

		memcpy(constBufPtr, &mvp, sizeof(DirectX::XMMATRIX));

		uint32_t x = app->getWindow()->getSize().x / 2;
		uint32_t y = app->getWindow()->getSize().y / 2;


		Rect rect;
		rect.pos = DirectX::XMFLOAT2(x, y);
		rect.size = DirectX::XMFLOAT2(200.0f, 200.0f);

		memcpy(rectBufPtr, &rect, sizeof(Rect));


		m_CmdList->SetGraphicsRootSignature(rootSignature.Get());
		m_CmdList->SetPipelineState(pipelineState.Get());


		m_CmdList->SetGraphicsRootConstantBufferView(0, constantBuf->GetGPUVirtualAddress());
		m_CmdList->SetGraphicsRootUnorderedAccessView(1, rectBuf->GetGPUVirtualAddress());


		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = vertexBuf->GetGPUVirtualAddress();
		vertexBufferView.SizeInBytes = sizeof(float) * 8;
		vertexBufferView.StrideInBytes = sizeof(float) * 2;

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = indexBuf->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = sizeof(int) * 6;

		m_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_CmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		m_CmdList->IASetIndexBuffer(&indexBufferView);
		m_CmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	uint32_t g_FenceValue = 0;

	void Renderer::endFrame() {
		Core::Application* app = Core::Application::getApplication();
		uint32_t frameIndex = app->getSwapchain()->getBackBufferIndex();
		
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

