#include "TextureManager.h"

#include "Core/Application.h"
#include "Core/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#include <fstream>

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

		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_UploadCmdAllocator));
		
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_UploadCmdAllocator, nullptr, IID_PPV_ARGS(&m_UploadCmdList));
		m_UploadCmdList->Close();

		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_UploadFence));

		m_UploadFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

	}

	TextureManager::~TextureManager()
	{

	}

	void TextureManager::beginTextureLoad()
	{
		m_UploadCmdAllocator->Reset();
		m_UploadCmdList->Reset(m_UploadCmdAllocator, nullptr);
	}

	void TextureManager::endTextureLoad()
	{
		Core::Application* app = Core::Application::getApplication();
		GraphicsContext* context = app->getGraphicsContext();

		m_UploadCmdList->Close();

		ID3D12CommandList* lists[] = { m_UploadCmdList };
		context->getQueue()->ExecuteCommandLists(1, lists);

		uint32_t fenceValue = ++m_UploadFenceValue;
		context->getQueue()->Signal(m_UploadFence, fenceValue);

		if (m_UploadFence->GetCompletedValue() < fenceValue) {
			m_UploadFence->SetEventOnCompletion(fenceValue, m_UploadFenceEvent);
			WaitForSingleObject(m_UploadFenceEvent, INFINITE);
		}

		for (auto* buffer : m_UploadBuffers)
			buffer->Release();

		m_UploadBuffers.clear();
	}

	uint32_t TextureManager::createTextureSet(std::string setName)
	{
		m_TextureSets.push_back(TextureSet{});
		m_TextureSets.back().setID = static_cast<uint32_t>(m_TextureSets.size() - 1);
		m_TextureSets.back().setName = setName;
		return m_TextureSets.back().setID;
	}

	struct TextureSetHeader {
		char magic[4];
		uint32_t setID;
		uint32_t textureCount;
	};

	void TextureManager::loadTextureSet(std::filesystem::path path)
	{
		std::ifstream file = std::ifstream(path.string().c_str(), std::ios::binary);

		TextureSetHeader hdr = {};
		file.read(reinterpret_cast<char*>(&hdr), sizeof(TextureSetHeader));
		if(strncmp(hdr.magic, "TXST", 4) != 0) {
			Core::Logger::Error("Invalid texture set file: {}", path.string());
			return;
		}

		for (auto it = m_TextureSets.begin(); it != m_TextureSets.end(); ++it) {
			if (it->setID == hdr.setID) {
				m_TextureSets.erase(it);
				break;
			}
		}

		TextureSet set{};
		set.setID = hdr.setID;
		set.textures.resize(hdr.textureCount);
		set.setName = path.stem().string();

		for (auto& tex : set.textures) {
			uint32_t nameSize = 0;
			file.read(reinterpret_cast<char*>(&nameSize), sizeof(uint32_t));
			tex.textureName.resize(nameSize);
			file.read(tex.textureName.data(), nameSize);
			uint32_t pathSize = 0;
			file.read(reinterpret_cast<char*>(&pathSize), sizeof(uint32_t));
			std::string pathStr(pathSize, '\0');
			file.read(pathStr.data(), pathSize);
			tex.texturePath = std::filesystem::path(pathStr);
			if (std::filesystem::exists(tex.texturePath) == true) {
				tex.textureID = loadTextureFromPath(tex.texturePath);
			}
			else Core::Logger::Warn("Missing Texture, {} not found.", tex.texturePath.string());
		}

		m_TextureSets.push_back(set);
	}

	void TextureManager::saveTextureSet(uint32_t setID, std::filesystem::path path)
	{
		TextureSet& set = getTextureSetByID(setID);

		TextureSetHeader hdr = {};
		hdr.magic[0] = 'T';
		hdr.magic[1] = 'X';
		hdr.magic[2] = 'S';
		hdr.magic[3] = 'T';
		hdr.setID = set.setID;
		hdr.textureCount = static_cast<uint32_t>(set.textures.size());

		if (std::filesystem::exists(path.parent_path()) == false) {
			std::filesystem::create_directories(path.parent_path());
		};

		std::ofstream file = std::ofstream(path.string().c_str(), std::ios::binary | std::ios::trunc);

		file.write(reinterpret_cast<char*>(&hdr), sizeof(TextureSetHeader));

		for (auto& entry : set.textures) {
			uint32_t nameSize = static_cast<uint32_t>(entry.textureName.size());
			file.write(reinterpret_cast<char*>(&nameSize), sizeof(uint32_t));
			file.write(entry.textureName.data(), nameSize);
			uint32_t pathSize = static_cast<uint32_t>(entry.texturePath.string().size());
			file.write(reinterpret_cast<char*>(&pathSize), sizeof(uint32_t));
			file.write(entry.texturePath.string().data(), pathSize);
		}

		file.close();

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
			beginTextureLoad();
			textureID = loadTextureFromPath(path);
			endTextureLoad();
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
			beginTextureLoad();
			textureID = loadTextureFromPath(newPath);
			endTextureLoad();
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

		m_UploadCmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		barrier.Transition.pResource = m_TextureResources.back();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		// Die Resource Barrier an die Command List senden
		m_UploadCmdList->ResourceBarrier(1, &barrier);

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
