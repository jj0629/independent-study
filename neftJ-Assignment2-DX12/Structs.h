#pragma once

#include "BufferStructs.h"
#include "Vertex.h"
#include <d3d12.h>

struct RtvSrvBundle
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE srvGPUHandle;
	Microsoft::WRL::ComPtr<ID3D12Resource> texResource;
	bool isScreenSized;
};

struct D3D12_HANDLE_BUNDLE
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};