#include "TextureManager.h"

#include "Core/Application.h"
#include "Core/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

namespace Graphics {
	TextureManager::TextureManager()
	{
		Core::Application* app = Core::Application::getApplication();
		auto device = app->getGraphicsContext()->getDevice();

		// Descriptor Heap für Shader Resource Views erstellen
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1024;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap));
		m_SRVHeapCPUStart = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();
		m_SRVHeapGPUStart = m_SRVHeap->GetGPUDescriptorHandleForHeapStart();
		m_SRVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	TextureManager::~TextureManager()
	{

	}

	void TextureManager::loadTextureSet(std::filesystem::path path)
	{

	}

	uint32_t TextureManager::createTextureSet()
	{
		m_TextureSets.push_back(TextureSet{});
		m_TextureSets.back().setID = static_cast<uint32_t>(m_TextureSets.size() - 1);
		return m_TextureSets.back().setID;
	}

	void TextureManager::saveTextureSet(uint32_t setID, std::filesystem::path path)
	{

	}

	TextureSet& TextureManager::getTextureSetByID(uint32_t setID)
	{


		for (auto& textureSet : m_TextureSets) {
			if (textureSet.setID == setID) {
				return textureSet;
				break;
			}
		}

		Core::Logger::Error("Texture set with ID {} not found.", setID);
		static TextureSet defaultSet{};
		return defaultSet;
	}

	void TextureManager::addTextureToSet(uint32_t setID, std::string name, std::filesystem::path path)
	{
		uint32_t textureID = 0;

		if (std::filesystem::exists(path) == true) {
			textureID = loadTextureFromPath(path);
		}
		TextureSet& set = getTextureSetByID(setID);

		TextureSetEntry entry{};
		entry.textureID = textureID;
		entry.textureName = name;
		entry.texturePath = path;

		set.textures.push_back(entry);
	}

	void TextureManager::modifyTextureInSet(uint32_t setID, std::string name, std::string newName, std::filesystem::path newPath)
	{
		TextureSet& set = getTextureSetByID(setID);

		uint32_t textureID = 0;

		if (std::filesystem::exists(newPath) == true) {
			textureID = loadTextureFromPath(newPath);
		}

		for (auto& entry : set.textures) {
			if (entry.textureName == name) {
				entry.textureName = newName;
				entry.textureID = textureID;
				entry.texturePath = newPath;
				return;
			}
		}
	}

	void TextureManager::removeTextureFromSet(uint32_t setID, std::string name)
	{
		TextureSet& set = getTextureSetByID(setID);

		for (auto it = set.textures.begin(); it != set.textures.end(); ++it) {
			if (it->textureName == name) {
				set.textures.erase(it);
				return;
			}
		}

	}



	uint32_t TextureManager::getTextureIDFromSet(uint32_t setID, std::string name)
	{
		TextureSet& set = getTextureSetByID(setID);

		for (auto& entry : set.textures) {
			if (entry.textureName == name) {
				return entry.textureID;
			}
		}

		return 0;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::getSRVDescriptorHandle(uint32_t index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_SRVHeapCPUStart;
		handle.ptr += m_SRVDescriptorSize * index;
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::getNextSRVDescriptorHandle(uint32_t& index) {
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_SRVHeapCPUStart;
		handle.ptr += m_SRVDescriptorSize * m_SRVHeapCurrentIndex;
		index = m_SRVHeapCurrentIndex;
		m_SRVHeapCurrentIndex++;
		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::getSRVGPUDescriptorHandle(uint32_t index)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVHeapGPUStart;
		handle.ptr += m_SRVDescriptorSize * index;
		return handle;
	}

	uint32_t TextureManager::loadTextureFromPath(std::filesystem::path path)
	{
		Core::Application* app = Core::Application::getApplication();
		int32_t width, height;
		int32_t channels;
		char* data = (char*)stbi_load(path.string().c_str(), &width, &height, &channels, 4);


		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = width;
		resourceDesc.Height = height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resourceDesc.SampleDesc = { 1, 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		m_TextureResources.push_back(nullptr);

		app->getGraphicsContext()->getDevice()->CreateCommittedResource(
			&heapProperties, D3D12_HEAP_FLAG_NONE,
			&resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr, IID_PPV_ARGS(&m_TextureResources.back())
		);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		UINT numRows = 0;
		UINT64 rowSizeBytes = 0;
		UINT64 uploadSize = 0;
		app->getGraphicsContext()->getDevice()->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &footprint, &numRows, &rowSizeBytes, &uploadSize);

		m_UploadBuffers.push_back(nullptr);
		m_UploadBuffers.back() = app->getRenderer()->createBuffer(uploadSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

		char* uploadData;
		m_UploadBuffers.back()->Map(0, nullptr, reinterpret_cast<void**>(&uploadData));

		for (uint32_t row = 0; row < numRows; row++) {
			memcpy(uploadData + footprint.Offset + row * footprint.Footprint.RowPitch, data + row * width * 4, width * 4);
		}

		m_UploadBuffers.back()->Unmap(0, nullptr);
		stbi_image_free(data);

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.pResource = m_UploadBuffers.back();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		dstLocation.pResource = m_TextureResources.back();
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex = 0;

		app->getRenderer()->getCmdList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
		app->getRenderer()->transition(m_TextureResources.back(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		uint32_t index = 0;
		app->getGraphicsContext()->getDevice()->CreateShaderResourceView(m_TextureResources.back(), &srvDesc, getNextSRVDescriptorHandle(index));

		return index;
	}
}
