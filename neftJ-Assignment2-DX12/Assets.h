#pragma once

#include <d3d12.h>
#include <string>
#include <codecvt>
#include <memory>
#include <unordered_map>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <fstream>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include "ResourceUploadBatch.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include <iostream>

#include "Mesh.h"
#include "Material.h"
#include "DX12Helper.h"
#include "Structs.h"


class Assets
{
#pragma region Singleton

public:
	static Assets& GetInstance()
	{
		if (!instance)
		{
			instance = new Assets();
		}

		return *instance;
	}

private:
	static Assets* instance;
	Assets() :
		allowOnDemandLoading(true),
		printLoadingProgress(false) {};

#pragma endregion

public:
	~Assets();

	void Initialize(
		std::string rootAssetPath,
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		bool allowOnDemandLoading = true,
		bool printLoadingProgress = false);

	// Getters
	std::shared_ptr<Mesh> GetMesh(std::string name);
	D3D12_CPU_DESCRIPTOR_HANDLE GetTexture(std::string name);
	std::shared_ptr<Material> GetMaterial(std::string name);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSig(std::string name);
	D3D12_STATIC_SAMPLER_DESC GetSampler(std::string name);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineStateObject(std::string name);
	Microsoft::WRL::ComPtr<ID3DBlob> GetVertexShaderBlob(std::string name);
	Microsoft::WRL::ComPtr<ID3DBlob> GetPixelShaderBlob(std::string name);
	RtvSrvBundle GetRenderTargetView(std::string name);
	std::unordered_map<std::string, RtvSrvBundle> GetAllRTVs();

	// Add methods
	void AddMesh(std::string name, std::shared_ptr<Mesh> mesh);
	void AddTexture(std::string name, D3D12_CPU_DESCRIPTOR_HANDLE tex);
	void AddMaterial(std::string name, std::shared_ptr<Material> mat);
	void AddRootSig(std::string name, Microsoft::WRL::ComPtr<ID3D12RootSignature> rs);
	void AddSampler(std::string name, D3D12_STATIC_SAMPLER_DESC sampler);
	void AddPipelineState(std::string name, Microsoft::WRL::ComPtr<ID3D12PipelineState> pso);
	void AddVertexShaderBlob(std::string name, Microsoft::WRL::ComPtr<ID3DBlob> vs);
	void AddPixelShaderBlob(std::string name, Microsoft::WRL::ComPtr<ID3DBlob> ps);
	void AddRenderTargetView(std::string name, RtvSrvBundle bundle);

	void ReleaseRTVs();
	void ReloadAllRTVs();

private:
	// Asset manager settings
	bool allowOnDemandLoading;
	bool printLoadingProgress;
	std::string rootAssetPath;

	// Other fields
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	std::vector<std::string> rtvReloadKeys;

	// Internal Unordered_Maps of data
	std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
	std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE> textures;
	std::unordered_map<std::string, std::shared_ptr<Material>> materials;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSignatures;
	std::unordered_map<std::string, D3D12_STATIC_SAMPLER_DESC> samplers;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStateObjects;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> vertexShaderBlobs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> pixelShaderBlobs;
	std::unordered_map<std::string, RtvSrvBundle> rtvSrvBundles;

	// Load methods
	std::shared_ptr<Mesh> LoadMesh(std::string path, std::string name);
	D3D12_CPU_DESCRIPTOR_HANDLE LoadTexture(std::string path, std::string name);
	D3D12_CPU_DESCRIPTOR_HANDLE LoadCubeMap(std::string path, std::string name);
	std::shared_ptr<Material> LoadMaterial(std::string path, std::string name);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> LoadRootSig(std::string path, std::string name);
	D3D12_STATIC_SAMPLER_DESC LoadSampler(std::string path, std::string name);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> LoadPipelineState(std::string path, std::string name);
	Microsoft::WRL::ComPtr<ID3DBlob> LoadVertexShaderBlob(std::string path, std::string name);
	Microsoft::WRL::ComPtr<ID3DBlob> LoadPixelShaderBlob(std::string path, std::string name);
	RtvSrvBundle LoadRtvSrvBundle(std::string path, std::string name);

	// Helpers for finding file paths
	std::string GetExePath();
	std::wstring GetExePath_Wide();

	std::string GetFullPathTo(std::string relativeFilePath);
	std::wstring GetFullPathTo_Wide(std::wstring relativeFilePath);

	bool EndsWith(std::string str, std::string ending);
	std::wstring ToWideString(std::string str);
	std::string RemoveFileExtension(std::string str);
};

