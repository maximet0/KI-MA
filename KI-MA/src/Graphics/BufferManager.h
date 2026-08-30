#pragma once
#include <d3d12.h>
#include <wrl.h>

#include <vector>
#include <array>

namespace Graphics {
	constexpr uint32_t frameCount = 2;

	typedef uint64_t BufferHandle;

	constexpr uint64_t heapChunkSize = 32 * 1024 * 1024; // 32 MB
	constexpr uint64_t defaultCopyHeapSize = 32 * 1024 * 1024; // 32 MB

	struct AllocationInfo {
		Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

		uint64_t offset = 0;
		uint64_t size = 0;

		uint32_t heapIndex = 0;
		uint32_t frameIndex = 0;
	};

	struct HeapChunk {
		Microsoft::WRL::ComPtr<ID3D12Heap> heap;
		uint64_t size = 0;
		uint64_t transientOffset = 0;
		bool transient = false;
	};

	struct FreeListEntry {
		uint32_t heapIndex = 0;
		uint64_t offset = 0;
		uint64_t size = 0;
	};

	struct CopyBuffer {
		Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
		char* mappedData = nullptr;

		uint64_t size = 0;
		uint64_t curOffset[frameCount];

		uint32_t frameIndex = 0;
	};

	struct MappedWrite {
		void* mappedMemory;
		BufferHandle handle;
		uint32_t size;
		uint64_t offset;
	};

	class BufferManager
	{
	public:
		BufferManager();
		~BufferManager();

		BufferHandle createBuffer(bool transient, size_t size);
		void destroyBuffer(BufferHandle handle);

		void clearOldBuffers(uint32_t frameIndex);

		MappedWrite beginMappedWrite(BufferHandle handle, uint32_t size);
		void submitMappedWrite(MappedWrite write, uint32_t bufOffset, uint32_t size);

		//void memset(BufferHandle handle, uint32_t value, uint32_t offset, uint32_t size);
		void write(BufferHandle handle, void* src, uint32_t offset, uint32_t size);

		void copy(BufferHandle srcBuffer, BufferHandle dstBufffer);
		void read(BufferHandle buffer, void* dst, uint32_t offset, uint32_t size);

		void transitionState(BufferHandle handle, D3D12_RESOURCE_STATES after);

		Microsoft::WRL::ComPtr<ID3D12Resource>& getBuffer(BufferHandle handle);

	private:
		uint32_t allocateHeapChunk(size_t size, bool transient);

		const BufferHandle TRANSIENT_HANDLE_MASK = 1ull << 63;

		uint64_t stagedWriteSize = 0;
		uint64_t stagedUploadOffset = 0;
		BufferHandle stagedBufferHandle = 0;

		std::vector<HeapChunk> m_Heaps;
		std::vector<FreeListEntry> m_FreeList;
		std::vector<BufferHandle> m_FreeHandles;

		std::vector<AllocationInfo> m_Buffers;
		std::vector<AllocationInfo> m_TransientBuffers;

		CopyBuffer m_UploadBuf;
		CopyBuffer m_ReadbackBuf;

		std::vector<CopyBuffer> m_OldCopyBuffers;

		std::vector<AllocationInfo> m_OldBuffers;


		uint64_t m_BufferHandleCounter = 1;
		uint64_t m_TransientBufferHandleCounter = TRANSIENT_HANDLE_MASK + 1;

	};

}
