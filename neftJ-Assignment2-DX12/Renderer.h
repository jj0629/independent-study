#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <stdlib.h>

#include "GameEntity.h"
#include "Sky.h"
#include "Camera.h"
#include "DX12Helper.h"
#include "Assets.h"
#include "Structs.h"
#include "EngineGUI.h"

class Renderer
{
public:
	Renderer(
		HWND hWnd,
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[],
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffers[],
		Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		unsigned int width,
		unsigned int height);
	~Renderer();

	void PreResize();
	void PostResize(unsigned int windowWidth,
		unsigned int windowHeight,
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffers[],
		Microsoft::WRL::ComPtr<ID3D12Resource> depthBufferDSV,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[],
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect);
	void Update(
		float deltaTime, float totalTime,
		std::vector<std::shared_ptr<GameEntity>> entities,
		std::shared_ptr<Sky> skyBox,
		std::vector<Light> lights,
		int lightCount);
	void Render(std::shared_ptr<Camera> camera, float deltaTime, float totalTime);

private:
	// Methods
	void SortEntityVectors();
	void ClearRenderTargets();

	// Rendering passes (as methods)
	void StandardEntities(std::shared_ptr<Camera> camera, float deltaTime, float totalTime);
	void PbrEntities(std::shared_ptr<Camera> camera, float deltaTime, float totalTime);
	void TransparentEntities(std::shared_ptr<Camera> camera, float deltaTime, float totalTime);
	void RefractiveEntities(std::shared_ptr<Camera> camera, float deltaTime, float totalTime);
	void Emitters(std::shared_ptr<Camera> camera, float deltaTime, float totalTime);
	void DepthOfField(std::shared_ptr<Camera> camera, float deltaTime, float totalTime);
	void FinalTextureToScreen(std::shared_ptr<Camera> camera, float deltaTime, float totalTime);

	// DX12 Fields
	bool vsync;
	static const unsigned int numBackBuffers = 2;
	unsigned int currentSwapBuffer;

	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[numBackBuffers];
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	Microsoft::WRL::ComPtr<ID3D12Resource> backBuffers[numBackBuffers];
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> currentRootSig;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> currentPSO;

	unsigned int width;
	unsigned int height;

	// Entities
	std::shared_ptr<Sky> skyBox;
	std::vector<Light> lights;
	unsigned int lightCount;

	std::vector<std::shared_ptr<GameEntity>> allEntities;
	std::vector<std::shared_ptr<GameEntity>> standardEntities;
	std::vector<std::shared_ptr<GameEntity>> pbrEntities;
	std::vector<std::shared_ptr<GameEntity>> transparentEntities;
	std::vector<std::shared_ptr<GameEntity>> refractiveEntities;

	// GUI
	std::shared_ptr<EngineGUI> engineGUI;
};