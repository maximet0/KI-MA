#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <stdint.h>
#include <DirectXMath.h>

#include "GraphicsPipeline.h"
#include "RenderTarget.h"

namespace Graphics {

	constexpr uint32_t frameCount = 2;

	constexpr uint32_t maxRects = 1000000;
	constexpr uint32_t maxLines = 1000000;


	struct RectData
	{
		DirectX::XMFLOAT2 pos;
		DirectX::XMFLOAT2 size;
		uint32_t textureID;
	};

	struct LineData
	{
		DirectX::XMFLOAT2 begin;
		uint32_t beginColor;
		DirectX::XMFLOAT2 end;
		uint32_t endColor;
	};


	class Renderer {
	public:
		Renderer();
		~Renderer();
	
		void initImGui();
		void renderImGui();

		void beginFrame();
		void endFrame();

		void beginRenderTarget(RenderTarget* renderTarget, DirectX::XMFLOAT2 cameraPosition, float cameraZoom);
		void endRenderTarget(RenderTarget* renderTarget);

		uint32_t createTexture(const char* texturePath);
		Microsoft::WRL::ComPtr<ID3D12Resource> createBuffer(size_t size, D3D12_HEAP_TYPE type, D3D12_RESOURCE_STATES initState);

		void submitRect(DirectX::XMFLOAT2 position, DirectX::XMFLOAT2 size, uint32_t textureHandle);
		void submitLine(DirectX::XMFLOAT2 begin, DirectX::XMFLOAT2 end, uint32_t color);

		void drawRects();
		void drawLines();
		void waitForGPU();

		D3D12_CPU_DESCRIPTOR_HANDLE getRTVDescriptorHandle(uint32_t index);
		D3D12_CPU_DESCRIPTOR_HANDLE getNextRTVDescriptorHandle(uint32_t& index);

		D3D12_CPU_DESCRIPTOR_HANDLE getSRVDescriptorHandle(uint32_t index);
		D3D12_CPU_DESCRIPTOR_HANDLE getNextSRVDescriptorHandle(uint32_t& index);

		D3D12_GPU_DESCRIPTOR_HANDLE getSRVGPUDescriptorHandle(uint32_t index);

	private:	

		void transition(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

		GraphicsPipeline m_DefaultPipeline;
		GraphicsPipeline m_LinePipeline;


		ID3D12GraphicsCommandList* m_CmdList;
		ID3D12CommandAllocator* m_CmdAllocators[frameCount];
		
		ID3D12Fence* m_Fence = nullptr;
		uint64_t m_FenceValues[frameCount] = {};
		HANDLE m_FenceEvent = nullptr;

		ID3D12DescriptorHeap* m_SRVHeap = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHeapCPUStart;
		D3D12_GPU_DESCRIPTOR_HANDLE m_SRVHeapGPUStart;
		uint32_t m_SRVDescriptorSize = 0;
		uint32_t m_SRVHeapCurrentIndex = 0;

		ID3D12DescriptorHeap* m_RTVHeap = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHeapCPUStart;
		uint32_t m_RTVDescriptorSize = 0;
		uint32_t m_RTVHeapCurrentIndex = 0;

		uint32_t m_CurrentRectOffset = 0;
		uint32_t m_CurrentRectCount = 0;

		uint32_t m_CurrentLineOffset = 0;
		uint32_t m_CurrentLineCount = 0;



		RenderTarget* m_CurrentRenderTarget = nullptr;

		uint32_t m_RectCount = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_RectDataBuf = nullptr;
		RectData* m_RectDataPtr = nullptr;

		uint32_t m_LineCount = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_LineDataBuf = nullptr;
		LineData* m_LineDataPtr = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> m_CurrentVPBuf = nullptr;

	};

}