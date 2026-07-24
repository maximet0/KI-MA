#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>



namespace Graphics {
	class RenderTarget {
	public:
		RenderTarget(DirectX::XMFLOAT2 size);
		~RenderTarget();

		void resize(DirectX::XMFLOAT2 size);

		DirectX::XMFLOAT2 getSize() { return m_Size; };

		ID3D12Resource* getRenderTarget() { return m_RenderTarget.Get(); };
		uint32_t getSRVDescriptorIndex() { return m_SRVDescriptorIndex; }
		uint32_t getRTVDescriptorIndex() { return m_RTVDescriptorIndex; }

	private:
		void createRenderTarget();

		DirectX::XMFLOAT2 m_Size;

		Microsoft::WRL::ComPtr<ID3D12Resource> m_RenderTarget = nullptr;
		uint32_t m_RTVDescriptorIndex = 0;
		uint32_t m_SRVDescriptorIndex = 0;
	};
}