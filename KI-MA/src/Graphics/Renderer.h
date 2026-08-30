#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <stdint.h>
#include <DirectXMath.h>

#include "GraphicsPipeline.h"
#include "RenderTarget.h"
#include "TextureManager.h"
#include "BufferManager.h"

namespace Graphics {

	//constexpr uint32_t frameCount = 2;

	constexpr uint32_t maxRects = 100000;
	constexpr uint32_t maxLines = 100000;


	struct RectData
	{
		DirectX::XMFLOAT2 pos;
		DirectX::XMFLOAT2 size;
		DirectX::XMFLOAT2 textureScale;
		float zLayer;
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

		void submitRect(DirectX::XMFLOAT2 position, float zLayer, DirectX::XMFLOAT2 size, uint32_t textureHandle, bool repeatTexture);
		void submitLine(DirectX::XMFLOAT2 begin, DirectX::XMFLOAT2 end, uint32_t color);

		void drawRects();
		void drawLines();
		void waitForGPU();

		D3D12_CPU_DESCRIPTOR_HANDLE getRTVDescriptorHandle(uint32_t index);
		D3D12_CPU_DESCRIPTOR_HANDLE getNextRTVDescriptorHandle(uint32_t& index);


		D3D12_CPU_DESCRIPTOR_HANDLE getDSVDescriptorHandle(uint32_t index);
		D3D12_CPU_DESCRIPTOR_HANDLE getNextDSVDescriptorHandle(uint32_t& index);



		void transition(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
		ID3D12GraphicsCommandList*& getCmdList() { return m_CmdList; }

		void beginCmdList(uint32_t frameIndex);
		void endCmdList();

		bool isCmdListOpen() { return cmdListOpen; }

		TextureManager& getTextureManager() { return m_TextureManager; }
		BufferManager& getBufferManager() { return m_BufferManager; }
	private:	


		GraphicsPipeline m_DefaultPipeline;
		GraphicsPipeline m_LinePipeline;

		TextureManager m_TextureManager;
		BufferManager m_BufferManager;

		bool cmdListOpen = false;
		ID3D12GraphicsCommandList* m_CmdList;
		ID3D12CommandAllocator* m_CmdAllocators[frameCount];
		
		ID3D12Fence* m_Fence = nullptr;
		uint64_t m_FenceValues[frameCount] = {};
		HANDLE m_FenceEvent = nullptr;

		ID3D12DescriptorHeap* m_RTVHeap = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHeapCPUStart;
		uint32_t m_RTVDescriptorSize = 0;
		uint32_t m_RTVHeapCurrentIndex = 0;

		ID3D12DescriptorHeap* m_DSVHeap = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHeapCPUStart;
		uint32_t m_DSVDescriptorSize = 0;
		uint32_t m_DSVHeapCurrentIndex = 0;

		uint32_t m_CurrentRectOffset = 0;
		uint32_t m_CurrentRectCount = 0;

		uint32_t m_CurrentLineOffset = 0;
		uint32_t m_CurrentLineCount = 0;

		RenderTarget* m_CurrentRenderTarget = nullptr;

		MappedWrite m_RectMappedMem;
		uint32_t m_RectCount = 0;
		BufferHandle m_RectDataBuf = 0;

		MappedWrite m_LineMappedMem;
		uint32_t m_LineCount = 0;
		BufferHandle m_LineDataBuf = 0;

		BufferHandle m_CurrentVPBuf = 0;

	};

}