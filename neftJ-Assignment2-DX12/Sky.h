#pragma once
#include <d3d12.h>
#include <memory>
#include "Mesh.h"
#include "Camera.h"
#include "Assets.h"
#include "BufferStructs.h"

class Sky
{
public:
	Sky(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect, D3D12_CPU_DESCRIPTOR_HANDLE cubeMap, std::shared_ptr<Mesh> mesh);
	~Sky();

	void CreateIBLResources();
	void Draw(std::shared_ptr<Camera> camera);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSkyCubeMap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetIrradianceMap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetConvolvedSpecularMap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetBRDFLookupTable();
	int GetConvolvedSpecularMipLevels();
	bool GetIBLCreationState();

private:

	void IBLCreateIrradianceMap();
	void IBLCreateConvolvedSpecularMap();
	void IBLCreateBRDFLookupTable();

	std::shared_ptr<Mesh> skyMesh;
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> skyDsvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> skyDepthStencilBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE skyDsvHandle;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	D3D12_CPU_DESCRIPTOR_HANDLE skyCubeMap;
	D3D12_GPU_DESCRIPTOR_HANDLE skyHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE irradianceCubeMap;
	D3D12_CPU_DESCRIPTOR_HANDLE convolvedSpecularMap;
	D3D12_CPU_DESCRIPTOR_HANDLE brdfLookupTable;

	int mipLevels;
	const int mipSkipLevels = 3;
	const int cubeMapSize = 512;
	const int textureSize = 512;
	bool isIBLCreated;
};

