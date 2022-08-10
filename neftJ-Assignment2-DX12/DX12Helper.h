#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"
#include "ResourceUploadBatch.h"
#include "Structs.h"
using namespace DirectX;


class DX12Helper
{
#pragma region Singleton
public:
	// Gets the one and only instance of this class
	static DX12Helper& GetInstance()
	{
		if (!instance)
		{
			instance = new DX12Helper();
		}

		return *instance;
	}

	// Remove these functions (C++ 11 version)
	DX12Helper(DX12Helper const&) = delete;
	void operator=(DX12Helper const&) = delete;

private:
	static DX12Helper* instance;
	DX12Helper() {};
#pragma endregion

public:
	~DX12Helper();

	// Initializing the Singleton
	void Initialize(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator,
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain);

	// Window dimension Setter/Getter
	void SetWidth(unsigned int w);
	void SetHeight(unsigned int h);

	unsigned int GetWidth();
	unsigned int GetHeight();

	// Creating Resources
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateStaticBuffer(
		unsigned int dataStride,
		unsigned int dataCount,
		void* data);

	// Command List and Synchronization
	void CloseExecuteAndResetCommandList();
	void WaitForGPU();

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetCBVSRVDescriptorHeap();
	D3D12_HANDLE_BUNDLE GetNextCBVSRVHeapLocationHandles();
	D3D12_GPU_DESCRIPTOR_HANDLE FillNextConstantBufferAndGetGPUDescriptorHandle(
		void* data,
		unsigned int dataSizeInBytes);

	// Texture stuffs
	D3D12_CPU_DESCRIPTOR_HANDLE LoadCubeMap(const wchar_t* file, bool generateMips = true);
	D3D12_CPU_DESCRIPTOR_HANDLE LoadTexture(const wchar_t* file, bool generateMips = true);
	D3D12_CPU_DESCRIPTOR_HANDLE LoadTexture(Microsoft::WRL::ComPtr<ID3D12Resource> texture);
	D3D12_GPU_DESCRIPTOR_HANDLE CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(
		D3D12_CPU_DESCRIPTOR_HANDLE firstDescriptorToCopy,
		unsigned int numDescriptorsToCopy);

	// MRT stuffs
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetRTVHeap();
	RtvSrvBundle CreateRtvSrvBundle(D3D12_RESOURCE_DESC texDesc, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, bool isScreenSize = true);
	RtvSrvBundle CreateRtvSrvBundle(Microsoft::WRL::ComPtr<ID3D12Resource> texture, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, bool isScreenSize = true);

private:
	// Actual device
	Microsoft::WRL::ComPtr<ID3D12Device> device;

	// Command list stuffs
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	
	// Swap chain
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;

	// Synchronization
	Microsoft::WRL::ComPtr<ID3D12Fence> waitFence;
	HANDLE waitFenceEvent;
	unsigned long waitFenceCounter;

	// Window dimensions
	unsigned int width;
	unsigned int height;

	// Maximum number of constant buffers, assuming each buffer
	// is 256 bytes or less.  Larger buffers are fine, but will
	// result in fewer buffers in use at any time
	const unsigned int maxConstantBuffers = 1000;
	const unsigned int maxRTVs = 1000;

	// GPU-side contant buffer upload heap
	Microsoft::WRL::ComPtr<ID3D12Resource> cbUploadHeap;
	UINT64 cbUploadHeapSizeInBytes;
	UINT64 cbUploadHeapOffsetInBytes;
	void* cbUploadHeapStartAddress;

	// GPU-side CBV/SRV descriptor heap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvSrvDescriptorHeap;
	SIZE_T cbvSrvDescriptorHeapIncrementSize;
	unsigned int cbvDescriptorOffset;

	void CreateConstantBufferUploadHeap();
	void CreateCBVSRVDescriptorHeap();
	void CreateRTVHeap();

	// Maximum number of texture descriptors (SRVs) we can have.
	// Each material will have a chunk of this,
	// Note: If we delayed the creation of this heap until 
	//       after all textures and materials were created,
	//       we could come up with an exact amount.  The following
	//       constant ensures we (hopefully) never run out of room.
	const unsigned int maxTextureDescriptors = 1000;
	unsigned int srvDescriptorOffset;
	// Texture resources we need to keep alive
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textures;
	std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> cpuSideTextureDescriptorHeaps;

	// MRT stuffs
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	unsigned int rtvDescriptorOffset;
	SIZE_T rtvDescriptorSize;
	std::vector< Microsoft::WRL::ComPtr<ID3D12Resource>> rtvPool;
};

