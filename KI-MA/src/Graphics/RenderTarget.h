#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>



namespace Graphics {
	class RenderTarget {
	public:
		RenderTarget(DirectX::XMFLOAT2 size, DirectX::XMFLOAT4 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f });
		~RenderTarget();

		void setClearColor(DirectX::XMFLOAT4 clearColor) { m_ClearColor = clearColor; };

		void resize(DirectX::XMFLOAT2 size);

		DirectX::XMFLOAT2 getSize() { return m_Size; };
		DirectX::XMFLOAT4 getClearColor() { return m_ClearColor; };

		ID3D12Resource* getRenderTarget() { return m_RenderTarget.Get(); };
		uint32_t getSRVDescriptorIndex() { return m_SRVDescriptorIndex; }
		uint32_t getRTVDescriptorIndex() { return m_RTVDescriptorIndex; }

	private:
		void createRenderTarget(DirectX::XMFLOAT4 clearColor);

		DirectX::XMFLOAT2 m_Size;
		DirectX::XMFLOAT4 m_ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };


		Microsoft::WRL::ComPtr<ID3D12Resource> m_RenderTarget = nullptr;
		uint32_t m_RTVDescriptorIndex = 0;
		uint32_t m_SRVDescriptorIndex = 0;
	};
}