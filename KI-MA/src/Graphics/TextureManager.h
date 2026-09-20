#pragma once

#include <DirectXMath.h>
#include <d3d12.h>
#include <stdint.h>
#include <vector>
#include <filesystem>
#include <string>



namespace Graphics {

	class Renderer;

	struct TextureSetEntry {
		uint32_t textureID;
		std::string textureName;
		std::filesystem::path texturePath;

		uint32_t runtimeIndex = 0;
	};

	struct TextureSet {
		uint32_t setID = 0;
		std::vector<TextureSetEntry> textures;
		std::string setName;
	};

	class TextureManager {
	public:
		TextureManager(Renderer* renderer);
		~TextureManager();

		void beginEarlyTextureLoad();
		void endEarlyTextureLoad();

		void loadTextureSet(std::filesystem::path path);

		uint32_t createTextureSet(std::string setName);
		void saveTextureSet(uint32_t setID, std::filesystem::path path);


		void addTextureToSet(uint32_t setID, std::string name, uint32_t textureID);

		void addTextureToSet(uint32_t setID, std::string name, std::filesystem::path path = "");
		void modifyTextureInSet(uint32_t setID, std::string name, std::string newName, std::filesystem::path newPath);

		void removeTextureFromSet(uint32_t setID, std::string name);
		uint32_t getTextureIDFromSet(uint32_t setID, std::string name);
		TextureSet& getTextureSetByID(uint32_t setID);

		DirectX::XMFLOAT2 getTextureSize(uint32_t textureID);

		std::vector<TextureSet>& getTextureSets() { return m_TextureSets; };

		D3D12_CPU_DESCRIPTOR_HANDLE getSRVDescriptorHandle(uint32_t index);
		D3D12_CPU_DESCRIPTOR_HANDLE getNextSRVDescriptorHandle(uint32_t& index);

		D3D12_GPU_DESCRIPTOR_HANDLE getSRVGPUDescriptorHandle(uint32_t index);

		ID3D12DescriptorHeap*& getSRVDescriptorHeap() { return m_SRVHeap; }

		uint32_t loadTextureFromPath(std::filesystem::path path);
		uint32_t loadTextureFromMemory(const char* bytes, int32_t width, int32_t height, int32_t channels);

	private:
		Renderer* m_Renderer = nullptr;
		ID3D12Fence* m_UploadFence = nullptr;
		uint32_t m_UploadFenceValue = 0;
		HANDLE m_UploadFenceEvent = nullptr;

		std::vector<TextureSet> m_TextureSets;

		std::vector<ID3D12Resource*> m_TextureResources;

		ID3D12DescriptorHeap* m_SRVHeap = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHeapCPUStart;
		D3D12_GPU_DESCRIPTOR_HANDLE m_SRVHeapGPUStart;
		uint32_t m_SRVDescriptorSize = 0;
		uint32_t m_SRVHeapCurrentIndex = 0;

	};
}
