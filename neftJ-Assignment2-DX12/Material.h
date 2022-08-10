#pragma once
#include "DX12Helper.h"
#include <DirectXMath.h>

class Material
{
public:
	Material(Microsoft::WRL::ComPtr<ID3D12RootSignature> rs, Microsoft::WRL::ComPtr<ID3D12PipelineState> ps, XMFLOAT3 color, XMFLOAT2 scale, XMFLOAT2 offset);
	~Material();
	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	void FinalizeMaterial();

	// Getters
	XMFLOAT3 GetColorTint();
	XMFLOAT2 GetUVScale();
	XMFLOAT2 GetUVOffset();
	bool GetIsFinalized();
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSig();
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForSRVs();

	// Setters
	void SetColorTint(XMFLOAT3 color);
	void SetUVScale(XMFLOAT2 scale);
	void SetUVOffset(XMFLOAT2 offset);
	void SetRootSig(Microsoft::WRL::ComPtr<ID3D12RootSignature> rs);
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> ps);

private:
	XMFLOAT3 colorTint;
	XMFLOAT2 uvScale;
	XMFLOAT2 uvOffset;
	bool finalized;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[4];
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;
};

