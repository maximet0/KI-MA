#include "BufferManager.h"

#include "Core/Application.h"
#include "Core/Logger.h"

namespace Graphics {

	BufferManager::BufferManager(Renderer* renderer)
		: m_Renderer(renderer)
	{

		auto& device = Core::Application::getApplication()->getGraphicsContext()->getDevice();

		m_Buffers.reserve(256);
		m_TransientBuffers.reserve(1024);


		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = defaultCopyHeapSize * frameCount;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc = { 1, 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;

		device->CreateCommittedResource(
			&heapDesc, D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
			IID_PPV_ARGS(&m_UploadBuf.buffer)
		);



		m_UploadBuf.buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_UploadBuf.mappedData));
		m_UploadBuf.size = defaultCopyHeapSize;

		heapDesc.Type = D3D12_HEAP_TYPE_READBACK;

		device->CreateCommittedResource(
			&heapDesc, D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_PPV_ARGS(&m_ReadbackBuf.buffer)
		);


		m_ReadbackBuf.buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_ReadbackBuf.mappedData));
		m_ReadbackBuf.size = defaultCopyHeapSize;

		for (uint32_t i = 0; i < frameCount; i++) {
			m_UploadBuf.curOffset[i] = defaultCopyHeapSize * i;
			m_ReadbackBuf.curOffset[i] = defaultCopyHeapSize * i;
		}

	}

	BufferManager::~BufferManager()
	{
		m_UploadBuf.buffer->Unmap(0, nullptr);
		m_ReadbackBuf.buffer->Unmap(0, nullptr);
	}

	uint64_t alignUp(uint64_t value, uint64_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	BufferHandle BufferManager::createBuffer(bool transient, size_t size)
	{
		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();
		BufferHandle handle = 0;

		AllocationInfo info;
		info.size = size;
		info.state = D3D12_RESOURCE_STATE_COMMON;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = size;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc = { 1, 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		

		auto allocInfo = device->GetResourceAllocationInfo(0, 1, &resourceDesc);

		if (allocInfo.SizeInBytes > heapChunkSize) {
			info.heapIndex = allocateHeapChunk(allocInfo.SizeInBytes, transient);
			info.offset = 0;
			device->CreatePlacedResource(m_Heaps[info.heapIndex].heap.Get(), info.offset, &resourceDesc, info.state, nullptr, IID_PPV_ARGS(&info.buffer));

			handle = transient ? m_TransientBufferHandleCounter++ : m_BufferHandleCounter++;
			BufferHandle rawHandle = handle - TRANSIENT_HANDLE_MASK;
			auto& buffers = transient ? m_TransientBuffers : m_Buffers;
			if (buffers.size() < rawHandle) buffers.resize(rawHandle);
			buffers[rawHandle - 1] = info;
		}
		else {
			bool foundFreeSpace = false;

			if (transient) {
				for (auto& chunk : m_Heaps) {
					uint64_t offset = alignUp(chunk.transientOffset, allocInfo.Alignment);

					if (chunk.transient && chunk.size - offset >= allocInfo.SizeInBytes) {
						info.heapIndex = &chunk - m_Heaps.data();
						info.offset = offset;
						chunk.transientOffset = offset + allocInfo.SizeInBytes;
						foundFreeSpace = true;
						break;
					}
				}

				if (!foundFreeSpace) {
					info.heapIndex = allocateHeapChunk(heapChunkSize, transient);
					info.offset = 0;
				}

				device->CreatePlacedResource(m_Heaps[info.heapIndex].heap.Get(), info.offset, &resourceDesc, info.state, nullptr, IID_PPV_ARGS(&info.buffer));

				handle = m_TransientBufferHandleCounter++;
				BufferHandle rawHandle = handle - TRANSIENT_HANDLE_MASK;
				if (m_TransientBuffers.size() < rawHandle) m_TransientBuffers.resize(rawHandle);
				m_TransientBuffers[rawHandle - 1] = info;
			}
			else {

				for (auto it = m_FreeList.begin(); it != m_FreeList.end(); ++it) {
					FreeListEntry& freeListEntry = *it;

					uint64_t offset = alignUp(freeListEntry.offset, allocInfo.Alignment);

					if (freeListEntry.size >= allocInfo.SizeInBytes) {
						info.heapIndex = freeListEntry.heapIndex;
						info.offset = offset;

						freeListEntry.offset = offset + allocInfo.SizeInBytes;
						freeListEntry.size = freeListEntry.size - allocInfo.SizeInBytes;
						foundFreeSpace = true;
						if (freeListEntry.size == 0) it = m_FreeList.erase(it);
						break;
					}
				}

				if (!foundFreeSpace) {
					info.heapIndex = allocateHeapChunk(heapChunkSize, transient);
					info.offset = 0;

					FreeListEntry freeEntry;
					freeEntry.heapIndex = info.heapIndex;
					freeEntry.offset = allocInfo.SizeInBytes;
					freeEntry.size = heapChunkSize - allocInfo.SizeInBytes;
					m_FreeList.push_back(freeEntry);
				}

				if (m_FreeHandles.size()) {
					handle = m_FreeHandles.back();
					m_FreeHandles.pop_back();

				}
				else {
					handle = ++m_BufferHandleCounter;
				}

				device->CreatePlacedResource(m_Heaps[info.heapIndex].heap.Get(), info.offset, &resourceDesc, info.state, nullptr, IID_PPV_ARGS(&info.buffer));

				if (m_Buffers.size() < handle) m_Buffers.resize(handle);
				m_Buffers[handle - 1] = info;
			}

		}

		return handle;
	}

	void BufferManager::destroyBuffer(BufferHandle handle)
	{
		if (m_Buffers.size() < handle || handle == 0 || handle & TRANSIENT_HANDLE_MASK) return;
		
		uint32_t frameIndex = Core::Application::getApplication()->getSwapchain()->getBackBufferIndex();

		m_Buffers[handle - 1].frameIndex = frameIndex;
		m_OldBuffers.push_back(m_Buffers[handle - 1]);

		//m_Buffers[handle - 1].buffer.Reset();
		m_FreeHandles.push_back(handle);
		m_FreeList.push_back({ m_Buffers[handle - 1].heapIndex, m_Buffers[handle - 1].offset, m_Buffers[handle - 1].size });
	}

	void BufferManager::clearOldBuffers(uint32_t frameIndex)
	{

		for (auto& chunk : m_Heaps) {
			if (chunk.transient) chunk.transientOffset = 0;
		}

		for (auto it = m_TransientBuffers.begin(); it != m_TransientBuffers.end(); ++it) {
			if (it->frameIndex == frameIndex) {
				it->buffer.Reset();
				it = m_TransientBuffers.erase(it);
				if (it == m_TransientBuffers.end()) break;
			}
		}

		m_TransientBufferHandleCounter = TRANSIENT_HANDLE_MASK + 1;
	
		for (auto& buf : m_OldCopyBuffers) {
			if (buf.frameIndex == frameIndex) {
				buf.buffer.Reset();
			}
		}

		m_UploadBuf.curOffset[frameIndex] = 0 + m_UploadBuf.size * frameIndex;
		m_ReadbackBuf.curOffset[frameIndex] = 0 + m_ReadbackBuf.size * frameIndex;

		for (auto& buf : m_OldBuffers) {
			if(buf.frameIndex == frameIndex) {
				buf.buffer.Reset();
			}
		}

	}

	MappedBuf BufferManager::beginMappedWrite(BufferHandle handle, uint32_t size)
	{
		MappedBuf write;

		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();
		uint32_t frameIndex = 0;
		if(Core::Application::getApplication()->getSwapchain() != nullptr) 
			frameIndex = Core::Application::getApplication()->getSwapchain()->getBackBufferIndex();

		auto& buffers = handle & TRANSIENT_HANDLE_MASK ? m_TransientBuffers : m_Buffers;
		BufferHandle rawHandle = handle & TRANSIENT_HANDLE_MASK ? handle & ~TRANSIENT_HANDLE_MASK : handle;

		if (rawHandle == 0 || rawHandle > buffers.size()) {
			Core::Logger::Error("BufferHandle invalid: {}", rawHandle);
			return MappedBuf();
		}

		write.handle = handle;
		write.size = size;
		
		if ((m_UploadBuf.curOffset[frameIndex] - m_UploadBuf.size * frameIndex) + size > m_UploadBuf.size) {
			Core::Logger::Warn("Slower Buffer Write: needed {} bytes, {} bytes available.", size, m_UploadBuf.size - (m_UploadBuf.curOffset[frameIndex] - m_UploadBuf.size * frameIndex));

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = (m_UploadBuf.size + size) * frameCount;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.SampleDesc = { 1, 0 };
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			D3D12_HEAP_PROPERTIES heapDesc = {};
			heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
			m_UploadBuf.frameIndex = frameIndex;
			m_UploadBuf.buffer->Unmap(0, nullptr);
			m_OldCopyBuffers.push_back(m_UploadBuf);

			device->CreateCommittedResource(
				&heapDesc, D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
				IID_PPV_ARGS(&m_UploadBuf.buffer)
			);

			m_UploadBuf.size = m_UploadBuf.size + size;
			m_UploadBuf.buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_UploadBuf.mappedData));
		}
		write.mappedMemory = m_UploadBuf.mappedData + m_UploadBuf.curOffset[frameIndex];
		write.offset = m_UploadBuf.curOffset[frameIndex];
		m_UploadBuf.curOffset[frameIndex] += size;
		return write;
	}

	MappedBuf BufferManager::beginMappedRead(BufferHandle handle, uint32_t size)
	{
		MappedBuf read;


		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();
		uint32_t frameIndex = 0;
		if (Core::Application::getApplication()->getSwapchain() != nullptr)
			frameIndex = Core::Application::getApplication()->getSwapchain()->getBackBufferIndex();

		auto& buffers = handle & TRANSIENT_HANDLE_MASK ? m_TransientBuffers : m_Buffers;
		BufferHandle rawHandle = handle & TRANSIENT_HANDLE_MASK ? handle & ~TRANSIENT_HANDLE_MASK : handle;

		if (rawHandle == 0 || rawHandle > buffers.size()) {
			Core::Logger::Error("BufferHandle invalid: {}", rawHandle);
			return MappedBuf();
		}

		read.handle = handle;
		read.size = size;

		if ((m_ReadbackBuf.curOffset[frameIndex] - m_ReadbackBuf.size * frameIndex) + size > m_ReadbackBuf.size) {
			Core::Logger::Warn("Slower Buffer Read: needed {} bytes, {} bytes available.", size, m_ReadbackBuf.size - (m_ReadbackBuf.curOffset[frameIndex] - m_ReadbackBuf.size * frameIndex));
			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = (m_ReadbackBuf.size + size) * frameCount;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.SampleDesc = { 1, 0 };
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			D3D12_HEAP_PROPERTIES heapDesc = {};
			heapDesc.Type = D3D12_HEAP_TYPE_READBACK;
			m_ReadbackBuf.frameIndex = frameIndex;
			m_ReadbackBuf.buffer->Unmap(0, nullptr);
			m_OldCopyBuffers.push_back(m_ReadbackBuf);
			device->CreateCommittedResource(
				&heapDesc, D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
				IID_PPV_ARGS(&m_ReadbackBuf.buffer)
			);
			m_ReadbackBuf.size = m_ReadbackBuf.size + size;
			m_ReadbackBuf.buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_ReadbackBuf.mappedData));
		}

		read.mappedMemory = m_ReadbackBuf.mappedData + m_ReadbackBuf.curOffset[frameIndex];
		read.offset = m_ReadbackBuf.curOffset[frameIndex];
		m_ReadbackBuf.curOffset[frameIndex] += size;

		return read;
	}

	void BufferManager::submitMappedRead(MappedBuf& read, uint32_t offset, uint32_t size)
	{
		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();
		uint32_t frameIndex = 0;
		if (Core::Application::getApplication()->getSwapchain() != nullptr)
			frameIndex = Core::Application::getApplication()->getSwapchain()->getBackBufferIndex();

		auto& buffers = read.handle & TRANSIENT_HANDLE_MASK ? m_TransientBuffers : m_Buffers;
		BufferHandle rawHandle = read.handle & TRANSIENT_HANDLE_MASK ? read.handle & ~TRANSIENT_HANDLE_MASK : read.handle;


		if (read.size < offset + size) {
			Core::Logger::Error("Buffer too small: {} bytes needed, {} bytes available.", size, read.size);
			return;
		}

		D3D12_RESOURCE_STATES oldState = buffers[rawHandle - 1].state;
		transitionState(read.handle, D3D12_RESOURCE_STATE_COPY_SOURCE);

		m_Renderer->getCmdList()->CopyBufferRegion(
			m_ReadbackBuf.buffer.Get(), read.offset,
			buffers[rawHandle - 1].buffer.Get(), offset,
			size
		);
		
		read.offset += size;

		transitionState(read.handle, oldState);

	}

	void BufferManager::executeReads()
	{
		m_Renderer->endCmdList();
		m_Renderer->waitForGPU();
		m_Renderer->beginCmdList(Core::Application::getApplication()->getSwapchain()->getBackBufferIndex());

		m_Renderer->getCmdList()->SetDescriptorHeaps(1, &m_Renderer->getTextureManager().getSRVDescriptorHeap());
	}

	void BufferManager::submitMappedWrite(MappedBuf& write, uint32_t bufOffset, uint32_t size)
	{
		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();
		uint32_t frameIndex = 0;
		if(Core::Application::getApplication()->getSwapchain() != nullptr)
			frameIndex = Core::Application::getApplication()->getSwapchain()->getBackBufferIndex();

		auto& buffers = write.handle & TRANSIENT_HANDLE_MASK ? m_TransientBuffers : m_Buffers;
		BufferHandle rawHandle = write.handle & TRANSIENT_HANDLE_MASK ? write.handle & ~TRANSIENT_HANDLE_MASK : write.handle;

		D3D12_RESOURCE_STATES oldState = buffers[rawHandle - 1].state;
		transitionState(write.handle, D3D12_RESOURCE_STATE_COPY_DEST);

		if (buffers[rawHandle - 1].size < bufOffset + size) {
			Core::Logger::Error("Buffer too small: {} bytes needed, {} bytes available.", size, buffers[rawHandle - 1].size);
			return;
		}

		m_Renderer->getCmdList()->CopyBufferRegion(
			buffers[rawHandle - 1].buffer.Get(), bufOffset,
			m_UploadBuf.buffer.Get(), write.offset,
			size
		);

		transitionState(write.handle, oldState);
	}

	void BufferManager::write(BufferHandle handle, void* src, uint32_t bufOffset, uint32_t size)
	{
		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();
		uint32_t frameIndex = Core::Application::getApplication()->getSwapchain()->getBackBufferIndex();

		auto& buffers = handle & TRANSIENT_HANDLE_MASK ? m_TransientBuffers : m_Buffers;
		BufferHandle rawHandle = handle & TRANSIENT_HANDLE_MASK ? handle & ~TRANSIENT_HANDLE_MASK : handle;

		if (rawHandle == 0 || rawHandle > buffers.size()) {
			Core::Logger::Error("BufferHandle invalid: {}", rawHandle);
			return;
		}

		if (buffers[rawHandle - 1].size < bufOffset + size) {
			Core::Logger::Error("Buffer too small: {} bytes needed, {} bytes available.", size, buffers[rawHandle - 1].size);
			return;
		}

		if ((m_UploadBuf.curOffset[frameIndex] - m_UploadBuf.size * frameIndex) + size > m_UploadBuf.size) {
			Core::Logger::Warn("Slower Buffer Write: needed {} bytes, {} bytes available.", size, m_UploadBuf.size - (m_UploadBuf.curOffset[frameIndex] - m_UploadBuf.size * frameIndex));

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = (m_UploadBuf.size + size) * frameCount;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.SampleDesc = { 1, 0 };
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			D3D12_HEAP_PROPERTIES heapDesc = {};
			heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
			m_UploadBuf.frameIndex = frameIndex;
			m_UploadBuf.buffer->Unmap(0, nullptr);
			m_OldCopyBuffers.push_back(m_UploadBuf);

			device->CreateCommittedResource(
				&heapDesc, D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
				IID_PPV_ARGS(&m_UploadBuf.buffer)
			);

			m_UploadBuf.size = m_UploadBuf.size + size;
			m_UploadBuf.buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_UploadBuf.mappedData));
		}

		uint32_t offset = m_UploadBuf.curOffset[frameIndex];

		memcpy(m_UploadBuf.mappedData + offset, src, size);
		m_UploadBuf.curOffset[frameIndex] += size;
		
		D3D12_RESOURCE_STATES oldState = buffers[rawHandle - 1].state;
		transitionState(handle, D3D12_RESOURCE_STATE_COPY_DEST);

		m_Renderer->getCmdList()->CopyBufferRegion(
			buffers[rawHandle-1].buffer.Get(), bufOffset,
			m_UploadBuf.buffer.Get(), offset,
			size
		);

		transitionState(handle, oldState);
	}

	void BufferManager::copy(BufferHandle srcBuffer, BufferHandle dstBuffer)
	{
		auto& srcBuffers = srcBuffer & TRANSIENT_HANDLE_MASK ? m_TransientBuffers : m_Buffers;
		auto& dstBuffers = dstBuffer & TRANSIENT_HANDLE_MASK ? m_TransientBuffers : m_Buffers;

		BufferHandle rawSrcHandle = srcBuffer & TRANSIENT_HANDLE_MASK ? srcBuffer & ~TRANSIENT_HANDLE_MASK : srcBuffer;
		BufferHandle rawDstHandle = dstBuffer & TRANSIENT_HANDLE_MASK ? dstBuffer & ~TRANSIENT_HANDLE_MASK : dstBuffer;

		if (srcBuffers.size() < rawSrcHandle || rawSrcHandle == 0) return;
		if (dstBuffers.size() < rawDstHandle || rawDstHandle == 0) return;

		if (srcBuffers[rawSrcHandle - 1].size > dstBuffers[rawDstHandle - 1].size) return;

		D3D12_RESOURCE_STATES srcOldState = srcBuffers[rawSrcHandle - 1].state;
		D3D12_RESOURCE_STATES dstOldState = dstBuffers[rawDstHandle - 1].state;

		transitionState(srcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
		transitionState(dstBuffer, D3D12_RESOURCE_STATE_COPY_DEST);

		m_Renderer->getCmdList()->CopyBufferRegion(
			dstBuffers[rawDstHandle - 1].buffer.Get(), 0,
			srcBuffers[rawSrcHandle - 1].buffer.Get(), 0,
			srcBuffers[rawSrcHandle - 1].size
		);

		transitionState(srcBuffer, srcOldState);
		transitionState(dstBuffer, dstOldState);
	}

	void BufferManager::transitionState(BufferHandle handle, D3D12_RESOURCE_STATES after)
	{	


		auto& buffers = handle & TRANSIENT_HANDLE_MASK ? m_TransientBuffers : m_Buffers;
		BufferHandle rawHandle = handle & TRANSIENT_HANDLE_MASK ? handle & ~TRANSIENT_HANDLE_MASK : handle;

		if (rawHandle == 0 || rawHandle > buffers.size()) {
			Core::Logger::Error("BufferHandle invalid: {}", rawHandle);
			return;
		}

		AllocationInfo& bufferInfo = buffers[rawHandle - 1];

		if (bufferInfo.state == after) {
			Core::Logger::Warn("Buffer transitioning to same State");
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		barrier.Transition.pResource = bufferInfo.buffer.Get();
		barrier.Transition.StateBefore = bufferInfo.state;
		barrier.Transition.StateAfter = after;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		bufferInfo.state = after;
		// Die Resource Barrier an die Command List senden
		m_Renderer->getCmdList()->ResourceBarrier(1, &barrier);
	}

	Microsoft::WRL::ComPtr<ID3D12Resource>& BufferManager::getBuffer(BufferHandle handle)
	{
		auto& buffers = handle & TRANSIENT_HANDLE_MASK ? m_TransientBuffers : m_Buffers;
		BufferHandle rawHandle = handle & TRANSIENT_HANDLE_MASK ? handle & ~TRANSIENT_HANDLE_MASK : handle;

		if (rawHandle == 0 || rawHandle > buffers.size()) {
			static Microsoft::WRL::ComPtr<ID3D12Resource> nullBuffer = nullptr;
			return nullBuffer;
		}

		return buffers[rawHandle - 1].buffer;
	}

	uint32_t BufferManager::allocateHeapChunk(size_t size, bool transient)
	{
		auto device = Core::Application::getApplication()->getGraphicsContext()->getDevice();
		HeapChunk newHeap;

		D3D12_HEAP_DESC heapDesc = {};
		heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapDesc.SizeInBytes = size;

		device->CreateHeap(&heapDesc, IID_PPV_ARGS(&newHeap.heap));
		newHeap.size = size;
		newHeap.transientOffset = 0;
		newHeap.transient = transient;

		m_Heaps.push_back(newHeap);
		return static_cast<uint32_t>(m_Heaps.size() - 1);
	}

}

