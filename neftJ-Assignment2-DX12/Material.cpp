#include "Material.h"

Material::Material(Microsoft::WRL::ComPtr<ID3D12RootSignature> rs, Microsoft::WRL::ComPtr<ID3D12PipelineState> ps, XMFLOAT3 color, XMFLOAT2 scale, XMFLOAT2 offset)
{
    this->rootSig = rs;
    this->pipelineState = ps;
    this->colorTint = color;
    this->uvScale = scale;
    this->uvOffset = offset;
    this->finalized = false;
}

Material::~Material()
{

}

void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot)
{
    // Make sure this material isn't finalized
    if (this->finalized) return;

    // Make sure slot is within the bounds of the array.
    if (slot < 0 || slot > 3) return;

    this->textureSRVsBySlot[slot] = srv;
}

void Material::FinalizeMaterial()
{
    if (this->finalized) return;

    for (int i = 0; i < 4; i++) 
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = DX12Helper::GetInstance().CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(this->textureSRVsBySlot[i], 1);

        if (i == 0) this->finalGPUHandleForSRVs = gpuHandle;
    }

    this->finalized = true;
}

#pragma region Getters

XMFLOAT3 Material::GetColorTint()
{
    return this->colorTint;
}

XMFLOAT2 Material::GetUVScale()
{
    return this->uvScale;
}

XMFLOAT2 Material::GetUVOffset()
{
    return this->uvOffset;
}

bool Material::GetIsFinalized()
{
    return this->finalized;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> Material::GetRootSig()
{
    return this->rootSig;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState()
{
    return this->pipelineState;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandleForSRVs()
{
    return this->finalGPUHandleForSRVs;
}

#pragma endregion

#pragma region Setters

void Material::SetColorTint(XMFLOAT3 color)
{
    this->colorTint = color;
}

void Material::SetUVScale(XMFLOAT2 scale)
{
    this->uvScale = scale;
}

void Material::SetUVOffset(XMFLOAT2 offset)
{
    this->uvOffset = offset;
}

void Material::SetRootSig(Microsoft::WRL::ComPtr<ID3D12RootSignature> rs)
{
    this->rootSig = rs;
}

void Material::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> ps)
{
    this->pipelineState = ps;
}

#pragma endregion