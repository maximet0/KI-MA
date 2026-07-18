#pragma once

#include "Core/Window.h"

#include <dxgi1_6.h>
#include <d3d12.h>

#include <wrl.h>

namespace Graphics {
	/// <summary>
	/// Enthält die grundlegenden DirectX 12 Objekte, die für die Grafikprogrammierung benötigt werden.
	/// </summary>
	class GraphicsContext {
	public:
		GraphicsContext();
		~GraphicsContext();

		/// <summary>
		/// Gibt die DirectX 12 Factory zurück, die für die Erstellung von Adaptern und Swapchains verwendet wird.
		/// </summary>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<IDXGIFactory7>& getFactory() { return m_Factory; };

		/// <summary>
		/// Gibt den DirectX 12 Adapter zurück, die für die Erstellung des Geräts verwendet wird.
		/// </summary>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<IDXGIAdapter4>& getAdapter() { return m_Adapter; };

		/// <summary>
		/// Gibt das DirectX 12 Gerät zurück, das für die Erstellung von Ressourcen und Pipelines verwendet wird.
		/// </summary>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<ID3D12Device14>& getDevice() { return m_Device; };

		/// <summary>
		/// Gibt die DirectX 12 Command Queue zurück, die für das Einreichen von Befehlen an die GPU verwendet wird.
		/// </summary>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>& getQueue() { return m_CommandQueue; };

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory7> m_Factory;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;
		Microsoft::WRL::ComPtr<ID3D12Device14> m_Device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
	};
}
