#pragma once

#include <d3d12.h>
#include <stdint.h>
#include <vector>
#include <filesystem>
#include <string>

namespace Graphics {


	struct TextureSetEntry {
		uint32_t textureID;
		std::string textureName;
		std::filesystem::path texturePath;
	};

	struct TextureSet {
		uint32_t setID = 0;
		std::vector<TextureSetEntry> textures;
	};

	class TextureManager {
	public:
		TextureManager();
		~TextureManager();

		void loadTextureSet(std::filesystem::path path);

		uint32_t createTextureSet();
		void saveTextureSet(uint32_t setID, std::filesystem::path path);

		void addTextureToSet(uint32_t setID, std::string name, std::filesystem::path path = "");
		void modifyTextureInSet(uint32_t setID, std::string name, std::string newName, std::filesystem::path newPath);

		void removeTextureFromSet(uint32_t setID, std::string name);
		uint32_t getTextureIDFromSet(uint32_t setID, std::string name);
		TextureSet& getTextureSetByID(uint32_t setID);

		std::vector<TextureSet>& getTextureSets() { return m_TextureSets; };

		D3D12_CPU_DESCRIPTOR_HANDLE getSRVDescriptorHandle(uint32_t index);
		D3D12_CPU_DESCRIPTOR_HANDLE getNextSRVDescriptorHandle(uint32_t& index);

		D3D12_GPU_DESCRIPTOR_HANDLE getSRVGPUDescriptorHandle(uint32_t index);

		ID3D12DescriptorHeap*& getSRVDescriptorHeap() { return m_SRVHeap; }

		uint32_t loadTextureFromPath(std::filesystem::path path);
	private:

		std::vector<TextureSet> m_TextureSets;

		std::vector<ID3D12Resource*> m_TextureResources;

		std::vector<ID3D12Resource*> m_UploadBuffers;

		ID3D12DescriptorHeap* m_SRVHeap = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHeapCPUStart;
		D3D12_GPU_DESCRIPTOR_HANDLE m_SRVHeapGPUStart;
		uint32_t m_SRVDescriptorSize = 0;
		uint32_t m_SRVHeapCurrentIndex = 0;

	};
}
