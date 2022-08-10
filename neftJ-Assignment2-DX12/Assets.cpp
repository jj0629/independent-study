#include "Assets.h"

// Singleton thingie
Assets* Assets::instance;

// Use this: https://en.cppreference.com/w/cpp/filesystem

Assets::~Assets()
{
    // Cleanup maps here
    meshes.clear();
    textures.clear();
    materials.clear();
    rootSignatures.clear();
    samplers.clear();
    pipelineStateObjects.clear();
    vertexShaderBlobs.clear();
    pixelShaderBlobs.clear();
}

void Assets::Initialize(std::string rootAssetPath, Microsoft::WRL::ComPtr<ID3D12Device> device, bool allowOnDemandLoading, bool printLoadingProgress)
{
    this->rootAssetPath = rootAssetPath;
    this->device = device;
    this->allowOnDemandLoading = allowOnDemandLoading;
    this->printLoadingProgress = printLoadingProgress;

    // cleanup root asset path
    std::replace(this->rootAssetPath.begin(), this->rootAssetPath.end(), '\\', '/');

    if (!EndsWith(this->rootAssetPath, "/")) this->rootAssetPath += "/";
}

std::shared_ptr<Mesh> Assets::GetMesh(std::string name)
{
    // See if the mesh is already loaded
    auto it = meshes.find(name);
    if (it != meshes.end())
        return it->second;

    // If not, load it in
    if (allowOnDemandLoading)
    {
        std::string filePath = GetFullPathTo(rootAssetPath + "Models\\" + name + ".obj");
        
        if (std::filesystem::exists(filePath))
        {
            return LoadMesh(filePath, name);
        }
    }

    // Failed
    return 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE Assets::GetTexture(std::string name)
{
    auto it = textures.find(name);
    if (it != textures.end())
        return it->second;

    if (allowOnDemandLoading)
    {
        std::string filePath;

        // Check for png first
        filePath = GetFullPathTo(rootAssetPath + "Textures\\" + name + ".png");
        if (std::filesystem::exists(filePath)) return LoadTexture(filePath, name);
        
        // Check for jpg next
        filePath = GetFullPathTo(rootAssetPath + "Textures\\" + name + ".jpg");
        if (std::filesystem::exists(filePath)) return LoadTexture(filePath, name);

        // Finally check for dds
        filePath = GetFullPathTo(rootAssetPath + "Textures\\" + name + ".dds");
        if (std::filesystem::exists(filePath)) return LoadCubeMap(filePath, name);
    }

    return LoadTexture("", name);
}

std::shared_ptr<Material> Assets::GetMaterial(std::string name)
{
    // See if the mesh is already loaded
    auto it = materials.find(name);
    if (it != materials.end())
        return it->second;

    // If not, load it in
    if (allowOnDemandLoading)
    {
        std::string filePath = GetFullPathTo(rootAssetPath + "Jsons\\Materials\\" + name + ".json");

        if (std::filesystem::exists(filePath))
        {
            return LoadMaterial(filePath, name);
        }
    }

    // Failed
    return 0;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> Assets::GetRootSig(std::string name)
{
    // First check if the root sig exists
    auto it = rootSignatures.find(name);
    if (it != rootSignatures.end())
        return it->second;

    if (allowOnDemandLoading)
    {
        std::string filePath = GetFullPathTo(rootAssetPath + "Jsons\\RootSigs\\" + name + ".json");
        if (std::filesystem::exists(filePath)) return LoadRootSig(filePath, name);
    }

    return Microsoft::WRL::ComPtr<ID3D12RootSignature>();
}

D3D12_STATIC_SAMPLER_DESC Assets::GetSampler(std::string name)
{
    auto it = samplers.find(name);
    if (it != samplers.end())
        return it->second;

    if (allowOnDemandLoading)
    {
        std::string filePath = GetFullPathTo(rootAssetPath + "Jsons\\Samplers\\" + name + ".json");
        if (std::filesystem::exists(filePath)) return LoadSampler(filePath, name);
    }
    return D3D12_STATIC_SAMPLER_DESC();
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Assets::GetPipelineStateObject(std::string name)
{
    auto it = pipelineStateObjects.find(name);
    if (it != pipelineStateObjects.end())
        return it->second;

    if (allowOnDemandLoading)
    {
        std::string filePath = GetFullPathTo(rootAssetPath + "Jsons\\PipelineStates\\" + name + ".json");
        if (std::filesystem::exists(filePath)) return LoadPipelineState(filePath, name);
    }

    return Microsoft::WRL::ComPtr<ID3D12PipelineState>();
}

Microsoft::WRL::ComPtr<ID3DBlob> Assets::GetVertexShaderBlob(std::string name)
{
    auto it = vertexShaderBlobs.find(name);
    if (it != vertexShaderBlobs.end())
        return it->second;

    if (allowOnDemandLoading) 
    {
        std::string filePath = GetFullPathTo(rootAssetPath + "..\\" + name + ".hlsl");
        if (std::filesystem::exists(filePath)) return LoadVertexShaderBlob(filePath, name);
    }
    return Microsoft::WRL::ComPtr<ID3DBlob>();
}

Microsoft::WRL::ComPtr<ID3DBlob> Assets::GetPixelShaderBlob(std::string name)
{
    auto it = pixelShaderBlobs.find(name);
    if (it != pixelShaderBlobs.end())
        return it->second;

    if (allowOnDemandLoading)
    {
        std::string filePath = GetFullPathTo(rootAssetPath + "..\\" + name + ".hlsl");
        if (std::filesystem::exists(filePath)) return LoadPixelShaderBlob(filePath, name);
    }

    return Microsoft::WRL::ComPtr<ID3DBlob>();
}

RtvSrvBundle Assets::GetRenderTargetView(std::string name)
{
    // Do this check first. If it's a reload, then we want to skip finding it in the dictionary and instead just remake it.
    auto it = rtvSrvBundles.find(name);
    if (it != rtvSrvBundles.end())
        return it->second;
    
    if (allowOnDemandLoading)
    {
        std::string filePath = GetFullPathTo(rootAssetPath + "Jsons\\RtvSrvBundles\\" + name + ".json");
        if (std::filesystem::exists(filePath)) return LoadRtvSrvBundle(filePath, name);
    }

    return RtvSrvBundle();
}

std::unordered_map<std::string, RtvSrvBundle> Assets::GetAllRTVs()
{
    return rtvSrvBundles;
}

void Assets::AddMesh(std::string name, std::shared_ptr<Mesh> mesh)
{
    // Make sure the key doesn't already exist
    if (meshes.find(name) == meshes.end())
    {
        meshes.insert({ name, mesh });
    }

    return;
}

void Assets::AddTexture(std::string name, D3D12_CPU_DESCRIPTOR_HANDLE tex)
{
    // Make sure the key doesn't already exist
    if (textures.find(name) == textures.end())
    {
        textures.insert({ name, tex });
    }

    return;
}

void Assets::AddMaterial(std::string name, std::shared_ptr<Material> mat)
{
    if (materials.find(name) == materials.end())
    {
        materials.insert({ name, mat });
    }

    return;
}

void Assets::AddRootSig(std::string name, Microsoft::WRL::ComPtr<ID3D12RootSignature> rs)
{
    // Make sure the key doesn't already exist
    if (rootSignatures.find(name) == rootSignatures.end())
    {
        rootSignatures.insert({ name, rs });
    }

    return;
}

void Assets::AddSampler(std::string name, D3D12_STATIC_SAMPLER_DESC sampler)
{
    // Make sure the key doesn't already exist
    if (samplers.find(name) == samplers.end())
    {
        samplers.insert({ name, sampler });
    }

    return;
}

void Assets::AddPipelineState(std::string name, Microsoft::WRL::ComPtr<ID3D12PipelineState> pso)
{
    // Make sure the key doesn't already exist
    if (pipelineStateObjects.find(name) == pipelineStateObjects.end())
    {
        pipelineStateObjects.insert({ name, pso });
    }

    return;
}

void Assets::AddVertexShaderBlob(std::string name, Microsoft::WRL::ComPtr<ID3DBlob> vs)
{
    // Make sure the key doesn't already exist
    if (vertexShaderBlobs.find(name) == vertexShaderBlobs.end())
    {
        vertexShaderBlobs.insert({ name, vs });
    }

    return;
}

void Assets::AddPixelShaderBlob(std::string name, Microsoft::WRL::ComPtr<ID3DBlob> ps)
{
    // Make sure the key doesn't already exist
    if (pixelShaderBlobs.find(name) == pixelShaderBlobs.end())
    {
        pixelShaderBlobs.insert({ name, ps });
    }

    return;
}

void Assets::AddRenderTargetView(std::string name, RtvSrvBundle bundle)
{
    // Make sure the key doesn't already exist
    if (rtvSrvBundles.find(name) == rtvSrvBundles.end())
    {
        rtvSrvBundles.insert({ name, bundle });
    }

    return;
}

void Assets::ReleaseRTVs()
{
    DX12Helper::GetInstance().WaitForGPU();
    std::unordered_map<std::string, RtvSrvBundle> temp;
    temp = rtvSrvBundles;
    
    for (const auto& [key, value] : temp) {
        if (value.isScreenSized)
        {
            rtvReloadKeys.push_back(key);
            rtvSrvBundles.erase(key);
        }
    }
}

void Assets::ReloadAllRTVs()
{
    for (const auto& key : rtvReloadKeys)
    {
        RtvSrvBundle temp = this->GetRenderTargetView(key);
    }

    rtvReloadKeys.clear();
}

std::shared_ptr<Mesh> Assets::LoadMesh(std::string path, std::string name)
{
    std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>(path.c_str());
    meshes.insert({ name, newMesh });
    return newMesh;
}

D3D12_CPU_DESCRIPTOR_HANDLE Assets::LoadTexture(std::string path, std::string name)
{
    D3D12_CPU_DESCRIPTOR_HANDLE tex = DX12Helper::GetInstance().LoadTexture(ToWideString(path).c_str());
    textures.insert({ name, tex });
    return tex;
}

D3D12_CPU_DESCRIPTOR_HANDLE Assets::LoadCubeMap(std::string path, std::string name)
{
    // Split out file names from name
    std::size_t lastSlash = name.find_last_of('\\');
    name = name.substr(lastSlash + 1, name.size());

    D3D12_CPU_DESCRIPTOR_HANDLE tex = DX12Helper::GetInstance().LoadCubeMap(ToWideString(path).c_str());
    textures.insert({ name, tex });
    return tex;
}

std::shared_ptr<Material> Assets::LoadMaterial(std::string path, std::string name)
{
    // READ JSON FILE HERE
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return nullptr;

    const std::size_t& size = std::filesystem::file_size(path);
    std::string content(size, '\0');
    file.read(content.data(), size);
    file.close();

    rapidjson::Document doc;
    doc.Parse(content.c_str());

    // Setup the structure of the document
    assert(doc.IsObject());
    {
        assert(doc["rsName"].IsString());
        assert(doc["psoName"].IsString());
        assert(doc["color"].IsArray());
        for (int i = 0; i < 3; i++)
        {
            assert(doc["color"][i].IsFloat());
        }
        assert(doc["scale"].IsArray());
        for (int i = 0; i < 2; i++)
        {
            assert(doc["scale"][i].IsFloat());
        }
        assert(doc["offset"].IsArray());
        for (int i = 0; i < 2; i++)
        {
            assert(doc["offset"][i].IsFloat());
        }
    }
    {
        assert(doc["textureCount"].IsInt());
        assert(doc["textures"].IsArray());
        for (int i = 0; i < doc["textureCount"].GetInt(); i++)
        {
            assert(doc["textures"][i].IsObject());
            assert(doc["textures"][i]["name"].IsString());
            assert(doc["textures"][i]["slot"].IsInt());
        }
    }

    // Actually create the material
    std::shared_ptr<Material> newMat = std::make_shared<Material>(
        GetRootSig(doc["rsName"].GetString()),
        GetPipelineStateObject(doc["psoName"].GetString()), 
        XMFLOAT3(doc["color"][0].GetFloat(), doc["color"][1].GetFloat(), doc["color"][2].GetFloat()), 
        XMFLOAT2(doc["scale"][0].GetFloat(), doc["scale"][1].GetFloat()),
        XMFLOAT2(doc["offset"][0].GetFloat(), doc["offset"][1].GetFloat()));
    for (int i = 0; i < doc["textureCount"].GetInt(); i++)
    {
        newMat->AddTexture(GetTexture(doc["textures"][i]["name"].GetString()), doc["textures"][i]["slot"].GetInt());
    }
    newMat->FinalizeMaterial();

    materials.insert({ name, newMat });

    return newMat;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> Assets::LoadRootSig(std::string path, std::string name)
{
    // READ JSON FILE HERE
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return nullptr;

    const std::size_t& size = std::filesystem::file_size(path);
    std::string content(size, '\0');
    file.read(content.data(), size);
    file.close();

    rapidjson::Document doc;
    doc.Parse(content.c_str());
    
    // Setup the structure of the document
    assert(doc.IsObject());

    // Descritpor ranges and information
    {
        assert(doc["descriptorRanges"].IsArray());
        for (int i = 0; i < doc["descriptorRanges"].Size(); i++)
        {
            assert(doc["descriptorRanges"][i].IsObject());
            assert(doc["descriptorRanges"][i]["type"].IsInt());
            assert(doc["descriptorRanges"][i]["descriptorNum"].IsInt());
            assert(doc["descriptorRanges"][i]["baseRegister"].IsInt());
            assert(doc["descriptorRanges"][i]["registerSpace"].IsInt());
        }
    }
    // Root parameter information
    {
        assert(doc["rootParams"].IsArray());
        for (int i = 0; i < doc["rootParams"].Size(); i++)
        {
            assert(doc["rootParams"][i].IsObject());
            assert(doc["rootParams"][i]["paramType"].IsInt());
            assert(doc["rootParams"][i]["shaderVisibility"].IsInt());
            assert(doc["rootParams"][i]["numDescriptors"].IsInt());
        }
    }
    // Sampler information
    {
        assert(doc["samplerNames"].IsArray());
        for (int i = 0; i < doc["samplerNames"].Size(); i++)
        {
            assert(doc["samplerNames"][i].IsString());
        }
    }

    // Make the root sig based on these fields
    D3D12_DESCRIPTOR_RANGE* descRanges = new D3D12_DESCRIPTOR_RANGE[doc["descriptorRanges"].Size()];
    for (int i = 0; i < doc["descriptorRanges"].Size(); i++)
    {
        descRanges[i].RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(doc["descriptorRanges"][i]["type"].GetInt());
        descRanges[i].NumDescriptors = doc["descriptorRanges"][i]["descriptorNum"].GetInt();
        descRanges[i].BaseShaderRegister = doc["descriptorRanges"][i]["baseRegister"].GetInt();
        descRanges[i].RegisterSpace = doc["descriptorRanges"][i]["registerSpace"].GetInt();
        descRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    D3D12_ROOT_PARAMETER* rootParams = new D3D12_ROOT_PARAMETER[doc["rootParams"].Size()];
    for (int i = 0; i < doc["rootParams"].Size(); i++)
    {
        rootParams[i].ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(doc["rootParams"][i]["paramType"].GetInt());
        rootParams[i].ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(doc["rootParams"][i]["shaderVisibility"].GetInt());
        rootParams[i].DescriptorTable.NumDescriptorRanges = doc["rootParams"][i]["numDescriptors"].GetInt();
        rootParams[i].DescriptorTable.pDescriptorRanges = &descRanges[i];
    }

    D3D12_STATIC_SAMPLER_DESC* samplers = new D3D12_STATIC_SAMPLER_DESC[doc["samplerNames"].Size()];
    for (int i = 0; i < doc["samplerNames"].Size(); i++)
    {
        D3D12_STATIC_SAMPLER_DESC temp = this->GetSampler(doc["samplerNames"][i].GetString());
        temp.ShaderRegister = i;
        samplers[i] = temp;
    }

    // Describe and serialize the root signature
    D3D12_ROOT_SIGNATURE_DESC rootSig = {};
    rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSig.NumParameters = doc["rootParams"].Size();
    rootSig.pParameters = rootParams;
    rootSig.NumStaticSamplers = doc["samplerNames"].Size();
    rootSig.pStaticSamplers = samplers;

    ID3DBlob* serializedRootSig = 0;
    ID3DBlob* errors = 0;

    D3D12SerializeRootSignature(
        &rootSig,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &errors);

    // Check for errors during serialization
    if (errors != 0)
    {
        OutputDebugString((char*)errors->GetBufferPointer());
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

    // Actually create the root sig
    device->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(rootSignature.GetAddressOf()));

    // Delete arrays
    delete[] descRanges;
    delete[] rootParams;
    delete[] samplers;

    rootSignatures.insert({ name, rootSignature });

    return rootSignature;
}

D3D12_STATIC_SAMPLER_DESC Assets::LoadSampler(std::string path, std::string name)
{
    // READ JSON FILE HERE
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return  D3D12_STATIC_SAMPLER_DESC();

    const std::size_t& size = std::filesystem::file_size(path);
    std::string content(size, '\0');
    file.read(content.data(), size);
    file.close();

    rapidjson::Document doc;
    doc.Parse(content.c_str());

    // Setup the doc and what it's fields are
    assert(doc.IsObject());
    assert(doc["addressU"].IsInt());
    assert(doc["addressV"].IsInt());
    assert(doc["addressW"].IsInt());
    assert(doc["filter"].IsInt());
    assert(doc["anisotropy"].IsInt());
    assert(doc["shaderVisibility"].IsInt());

    // Create the sampler descriptor
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(doc["addressU"].GetInt());
    sampler.AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(doc["addressV"].GetInt());
    sampler.AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(doc["addressW"].GetInt());
    sampler.Filter = static_cast<D3D12_FILTER>(doc["filter"].GetInt());
    sampler.MaxAnisotropy = doc["anisotropy"].GetInt();
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(doc["shaderVisibility"].GetInt());

    samplers.insert({ name, sampler });

    return sampler;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Assets::LoadPipelineState(std::string path, std::string name)
{
    // READ JSON FILE HERE
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return nullptr;

    const std::size_t& size = std::filesystem::file_size(path);
    std::string content(size, '\0');
    file.read(content.data(), size);
    file.close();

    rapidjson::Document doc;
    doc.Parse(content.c_str());

    // Tell our code what the document's structure is like
    assert(doc.IsObject());
    // Shader information
    {
        assert(doc["rootSigName"].IsString());
        assert(doc["vsName"].IsString());
        assert(doc["psName"].IsString());
    }
    // Input element info
    {
        assert(doc["inputElements"].IsArray());
        for (int i = 0; i < doc["inputElements"].Size(); i++)
        {
            assert(doc["inputElements"][i].IsObject());
            assert(doc["inputElements"][i]["format"].IsInt());
            assert(doc["inputElements"][i]["semanticName"].IsString());
            assert(doc["inputElements"][i]["index"].IsInt());
        }
    }
    // Render target information (includes blend states for each one!)
    {
        assert(doc["renderTargetFormats"].IsArray());
        assert(doc["blendStates"].IsArray());
        for (int i = 0; i < doc["renderTargetFormats"].Size(); i++)
        {
            assert(doc["renderTargetFormats"][i].IsInt());

            assert(doc["blendStates"][i].IsObject());
            assert(doc["blendStates"][i]["srcBlend"].IsInt());
            assert(doc["blendStates"][i]["destBlend"].IsInt());
            assert(doc["blendStates"][i]["blendOp"].IsInt());
            assert(doc["blendStates"][i]["writeMask"].IsInt());
        }
    }
    // Misc variables
    {
        assert(doc["dsvFormat"].IsInt());
        assert(doc["samplerCount"].IsInt());
        assert(doc["samplerQuality"].IsInt());
    }
    // The rasterizer state information
    {
        assert(doc["rasterizerState"].IsObject());
        assert(doc["rasterizerState"]["fill"].IsInt());
        assert(doc["rasterizerState"]["cull"].IsInt());
        assert(doc["rasterizerState"]["depthClip"].IsBool());
    }
    // The depth stencil information.
    {
        assert(doc["depthStencil"].IsObject());
        assert(doc["depthStencil"]["depthEnable"].IsBool());
        assert(doc["depthStencil"]["depthFunc"].IsInt());
        assert(doc["depthStencil"]["writeMask"].IsInt());
    }

    // Actually create the pso here
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    // -- Input assembler related ---
    D3D12_INPUT_ELEMENT_DESC* inputElements = new D3D12_INPUT_ELEMENT_DESC[doc["inputElements"].Size()];
    for (int i = 0; i < doc["inputElements"].Size(); i++)
    {
        D3D12_INPUT_ELEMENT_DESC newDesc = {};
        newDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        newDesc.Format = static_cast<DXGI_FORMAT>(doc["inputElements"][i]["format"].GetInt());
        newDesc.SemanticName = doc["inputElements"][i]["semanticName"].GetString();
        newDesc.SemanticIndex = doc["inputElements"][i]["index"].GetInt();
        inputElements[i] = newDesc;
    }
    
    psoDesc.InputLayout.NumElements = doc["inputElements"].Size();
    psoDesc.InputLayout.pInputElementDescs = &inputElements[0];
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Root sig
    psoDesc.pRootSignature = GetRootSig(doc["rootSigName"].GetString()).Get();

    // -- Shaders (VS/PS) --- 
    psoDesc.VS.pShaderBytecode = GetPixelShaderBlob(doc["vsName"].GetString())->GetBufferPointer();
    psoDesc.VS.BytecodeLength = GetPixelShaderBlob(doc["vsName"].GetString())->GetBufferSize();
    psoDesc.PS.pShaderBytecode = GetPixelShaderBlob(doc["psName"].GetString())->GetBufferPointer();
    psoDesc.PS.BytecodeLength = GetPixelShaderBlob(doc["psName"].GetString())->GetBufferSize();

    // -- Render targets ---
    psoDesc.NumRenderTargets = doc["renderTargetFormats"].Size();
    for (int i = 0; i < psoDesc.NumRenderTargets; i++)
    {
        psoDesc.RTVFormats[i] = static_cast<DXGI_FORMAT>(doc["renderTargetFormats"][i].GetInt());

        // Blend States
        psoDesc.BlendState.RenderTarget[i].SrcBlend = static_cast<D3D12_BLEND>(doc["blendStates"][i]["srcBlend"].GetInt());
        psoDesc.BlendState.RenderTarget[i].DestBlend = static_cast<D3D12_BLEND>(doc["blendStates"][i]["destBlend"].GetInt());
        psoDesc.BlendState.RenderTarget[i].BlendOp = static_cast<D3D12_BLEND_OP>(doc["blendStates"][i]["blendOp"].GetInt());
        psoDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = static_cast<D3D12_COLOR_WRITE_ENABLE>(doc["blendStates"][i]["writeMask"].GetInt());
    }
    psoDesc.DSVFormat = static_cast<DXGI_FORMAT>(doc["dsvFormat"].GetInt());
    psoDesc.SampleDesc.Count = doc["samplerCount"].GetInt();
    psoDesc.SampleDesc.Quality = doc["samplerQuality"].GetInt();

    // -- States ---
    psoDesc.RasterizerState.FillMode = static_cast<D3D12_FILL_MODE>(doc["rasterizerState"]["fill"].GetInt());
    psoDesc.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(doc["rasterizerState"]["cull"].GetInt());
    psoDesc.RasterizerState.DepthClipEnable = doc["rasterizerState"]["depthClip"].GetBool();

    psoDesc.DepthStencilState.DepthEnable = doc["depthStencil"]["depthEnable"].GetBool();
    psoDesc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(doc["depthStencil"]["depthFunc"].GetInt());
    psoDesc.DepthStencilState.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(doc["depthStencil"]["writeMask"].GetInt());

    // -- Misc ---
    psoDesc.SampleMask = 0xffffffff;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

    // Create the pipe state object
    device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipelineState.GetAddressOf()));
    
    pipelineStateObjects.insert({ name, pipelineState });

    delete[] inputElements;

    return pipelineState;
}

Microsoft::WRL::ComPtr<ID3DBlob> Assets::LoadVertexShaderBlob(std::string path, std::string name)
{
    Microsoft::WRL::ComPtr<ID3DBlob> temp;

    D3DReadFileToBlob(GetFullPathTo_Wide(ToWideString(name) + L".cso").c_str(), temp.GetAddressOf());

    vertexShaderBlobs.insert({ name, temp });

    return temp;
}

Microsoft::WRL::ComPtr<ID3DBlob> Assets::LoadPixelShaderBlob(std::string path, std::string name)
{
    Microsoft::WRL::ComPtr<ID3DBlob> temp;

    D3DReadFileToBlob(GetFullPathTo_Wide(ToWideString(name) + L".cso").c_str(), temp.GetAddressOf());

    pixelShaderBlobs.insert({ name, temp });

    return temp;
}

RtvSrvBundle Assets::LoadRtvSrvBundle(std::string path, std::string name)
{
    // READ JSON FILE HERE
        std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return RtvSrvBundle();

    const std::size_t& size = std::filesystem::file_size(path);
    std::string content(size, '\0');
    file.read(content.data(), size);
    file.close();

    rapidjson::Document doc;
    doc.Parse(content.c_str());

    // Setup the structure of the document
    assert(doc.IsObject());

    // Initialize texDesc variables in doc
    {
        assert(doc["texDesc"].IsObject());
        assert(doc["texDesc"]["dimension"].IsInt());
        assert(doc["texDesc"]["depth"].IsInt());
        assert(doc["texDesc"]["format"].IsInt());
        assert(doc["texDesc"]["mipLevels"].IsInt());
        assert(doc["texDesc"]["samplerCount"].IsInt());
    }
    // Initialize rtvDesc variables in doc
    {
        assert(doc["rtvDesc"].IsObject());
        assert(doc["rtvDesc"]["viewDimension"].IsInt());
        assert(doc["rtvDesc"]["numElements"].IsInt());
    }
    assert(doc["isScreenSize"].IsBool());

    // If it isn't screen sized, then grab the correct dimensions
    if (doc["isScreenSize"].GetBool() == false)
    {
        assert(doc["width"].IsInt());
        assert(doc["height"].IsInt());
    }

    // Create the texture desc
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(doc["texDesc"]["dimension"].GetInt());
    texDesc.DepthOrArraySize = doc["texDesc"]["depth"].GetInt();
    texDesc.Width = DX12Helper::GetInstance().GetWidth();
    texDesc.Height = DX12Helper::GetInstance().GetHeight();
    texDesc.Format = static_cast<DXGI_FORMAT>(doc["texDesc"]["format"].GetInt());
    texDesc.MipLevels = doc["texDesc"]["mipLevels"].GetInt();
    texDesc.SampleDesc.Count = doc["texDesc"]["samplerCount"].GetInt();
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    // Create the rtv desc
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = texDesc.Format;
    rtvDesc.Texture2D.MipSlice = texDesc.MipLevels;
    rtvDesc.ViewDimension = static_cast<D3D12_RTV_DIMENSION>(doc["rtvDesc"]["viewDimension"].GetInt());
    rtvDesc.Buffer.NumElements = doc["rtvDesc"]["numElements"].GetInt();

    bool isScreenSize = doc["isScreenSize"].GetBool();

    if (isScreenSize == false)
    {
        texDesc.Width = doc["width"].GetInt();
        texDesc.Height = doc["height"].GetInt();
    }

    // Call the DX12Helper methods
    RtvSrvBundle payload = DX12Helper::GetInstance().CreateRtvSrvBundle(texDesc, rtvDesc, isScreenSize);
    
    rtvSrvBundles.insert({ name, payload });

    return payload;
}

std::string Assets::GetExePath()
{
    std::string path = ".\\";
    char currentDir[1024] = {};
    GetModuleFileName(0, currentDir, 1024);

    char* lastSlash = strrchr(currentDir, '\\');
    if (lastSlash)
    {
        *lastSlash = 0;

        path = currentDir;
    }

    return path;
}

std::wstring Assets::GetExePath_Wide()
{
    std::string path = GetExePath();

    wchar_t widePath[1024] = {};
    mbstowcs_s(0, widePath, path.c_str(), 1024);

    return std::wstring(widePath);
}

std::string Assets::GetFullPathTo(std::string relativeFilePath)
{
    return GetExePath() +  "\\" + relativeFilePath;
}

std::wstring Assets::GetFullPathTo_Wide(std::wstring relativeFilePath)
{
    return GetExePath_Wide() + L"\\" + relativeFilePath;
}

bool Assets::EndsWith(std::string str, std::string ending)
{
    return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

std::wstring Assets::ToWideString(std::string str)
{
    std::wstring temp(str.begin(), str.end());
    return temp;
}

std::string Assets::RemoveFileExtension(std::string str)
{
    size_t found = str.find_last_of('.');
    return str.substr(0, found);
}