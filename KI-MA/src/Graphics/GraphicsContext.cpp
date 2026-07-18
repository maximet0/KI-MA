#include "GraphicsContext.h"

#include <stdexcept>
#include <wrl.h>

namespace Graphics {
	GraphicsContext::GraphicsContext() {
		HRESULT res;
		UINT flags = 0;
#ifdef DEBUG
		// Debug Layer aktivieren
		ID3D12Debug* debugInterface;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
		debugInterface->EnableDebugLayer();
		flags = DXGI_CREATE_FACTORY_DEBUG;
#endif // DEBUG

		// DXGI Factory erstellen
		res = CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_Factory));
		if (FAILED(res)) throw std::runtime_error("");

		// Adapter auswählen, erster Adapter mit hoher Leistung wird gewählt
		DXGI_ADAPTER_DESC3 adapterDesc = {};
		for (uint32_t i = 0; m_Factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_Adapter)) != DXGI_ERROR_NOT_FOUND; i++) {
			break;
		}

		// DirectX 12 Gerät erstellen
		res = D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device));
		if (FAILED(res)) throw std::runtime_error("");

		// Command Queue erstellen
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
		res = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue));
		if (FAILED(res)) throw std::runtime_error("");

#ifdef DEBUG
		// Debugging-Informationen aktivieren
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		m_Device.As(&infoQueue);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
#endif
	}

	GraphicsContext::~GraphicsContext() {
		// Alle Ressourcen freigeben
		m_CommandQueue->Release();
		m_Device->Release();
		m_Adapter->Release();
		m_Factory->Release();
	}
}