#include "Sky.h"

Sky::Sky(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect, D3D12_CPU_DESCRIPTOR_HANDLE cubeMap, std::shared_ptr<Mesh> mesh)
{
    this->device = device;
    this->commandList = commandList;
    this->dsvHandle = dsvHandle;
    this->viewport = viewport;
    this->scissorRect = scissorRect;
    this->skyCubeMap = cubeMap;
    this->skyMesh = mesh;
    this->skyHandle = DX12Helper::GetInstance().CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(this->skyCubeMap, 1);
    this->isIBLCreated = false;

    // Create depth/stencil buffer
    {
        // Create a descriptor heap for DSV
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(skyDsvHeap.GetAddressOf()));

        // Describe the depth stencil buffer resource
        D3D12_RESOURCE_DESC depthBufferDesc = {};
        depthBufferDesc.Alignment = 0;
        depthBufferDesc.DepthOrArraySize = 1;
        depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthBufferDesc.Height = textureSize;
        depthBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthBufferDesc.MipLevels = 1;
        depthBufferDesc.SampleDesc.Count = 1;
        depthBufferDesc.SampleDesc.Quality = 0;
        depthBufferDesc.Width = textureSize;

        // Describe the clear value that will most often be used
        // for this buffer (which optimizes the clearing of the buffer)
        D3D12_CLEAR_VALUE clear = {};
        clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clear.DepthStencil.Depth = 1.0f;
        clear.DepthStencil.Stencil = 0;

        // Describe the memory heap that will house this resource
        D3D12_HEAP_PROPERTIES props = {};
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.CreationNodeMask = 1;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        props.Type = D3D12_HEAP_TYPE_DEFAULT;
        props.VisibleNodeMask = 1;

        // Actually create the resource, and the heap in which it
        // will reside, and map the resource to that heap
        device->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &depthBufferDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clear,
            IID_PPV_ARGS(skyDepthStencilBuffer.GetAddressOf()));

        // Get the handle to the Depth Stencil View that we'll
        // be using for the depth buffer.  The DSV is stored in
        // our DSV-specific descriptor Heap.
        skyDsvHandle = skyDsvHeap->GetCPUDescriptorHandleForHeapStart();

        // Actually make the DSV
        device->CreateDepthStencilView(
            skyDepthStencilBuffer.Get(),
            0,	// Default view (first mip)
            skyDsvHandle);
    }
}

Sky::~Sky()
{
}

void Sky::CreateIBLResources()
{
    IBLCreateIrradianceMap();
    DX12Helper::GetInstance().WaitForGPU();
    IBLCreateConvolvedSpecularMap();
    DX12Helper::GetInstance().WaitForGPU();
    IBLCreateBRDFLookupTable();
    DX12Helper::GetInstance().WaitForGPU();

    isIBLCreated = true;
}

void Sky::Draw(std::shared_ptr<Camera> camera)
{
    commandList->SetGraphicsRootSignature(Assets::GetInstance().GetRootSig("skyRS").Get());
    commandList->SetPipelineState(Assets::GetInstance().GetPipelineStateObject("skyPSO").Get());

    SkyVSData vsData = {};
    vsData.View = camera->GetView();
    vsData.Projection = camera->GetProjection();

    D3D12_GPU_DESCRIPTOR_HANDLE cbHandleVS = DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&vsData), sizeof(VertexShaderExternalData));
    commandList->SetGraphicsRootDescriptorTable(0, cbHandleVS);

    commandList->SetGraphicsRootDescriptorTable(1, skyHandle);

    D3D12_VERTEX_BUFFER_VIEW vbv = skyMesh->GetVertexBuffer();
    D3D12_INDEX_BUFFER_VIEW ibv = skyMesh->GetIndexBuffer();

    commandList->IASetVertexBuffers(0, 1, &vbv);
    commandList->IASetIndexBuffer(&ibv);

    commandList->DrawIndexedInstanced(skyMesh->GetIndexCount(), 1, 0, 0, 0);
}

D3D12_CPU_DESCRIPTOR_HANDLE Sky::GetSkyCubeMap()
{
    return this->skyCubeMap;
}

D3D12_CPU_DESCRIPTOR_HANDLE Sky::GetIrradianceMap()
{
    return this->irradianceCubeMap;
}

D3D12_CPU_DESCRIPTOR_HANDLE Sky::GetConvolvedSpecularMap()
{
    return this->convolvedSpecularMap;
}

D3D12_CPU_DESCRIPTOR_HANDLE Sky::GetBRDFLookupTable()
{
    return this->brdfLookupTable;
}

int Sky::GetConvolvedSpecularMipLevels()
{
    return this->mipLevels;
}

bool Sky::GetIBLCreationState()
{
    return this->isIBLCreated;
}

void Sky::IBLCreateIrradianceMap()
{
    Microsoft::WRL::ComPtr<ID3D12Resource> irrMapFinalTexture;
    // Create the final irradiance cube texture
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Width = cubeMapSize; 			// One of your constants
    texDesc.Height = cubeMapSize; 			// Same as width
    texDesc.DepthOrArraySize = 6; 					// Cube map means 6 textures
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // Will be used as both
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 		// Basic texture format
    texDesc.MipLevels = 1; 					// No mip chain needed
    texDesc.SampleDesc.Count = 1; 				// Can't be zero
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    D3D12_HEAP_PROPERTIES heapDesc = {};
    heapDesc.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
    device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &texDesc, D3D12_RESOURCE_STATE_COMMON | D3D12_RESOURCE_STATE_RENDER_TARGET, 0, IID_PPV_ARGS(irrMapFinalTexture.GetAddressOf()));

    D3D12_VIEWPORT newViewport = {};
    newViewport.Width = textureSize;
    newViewport.Height = textureSize;
    newViewport.MinDepth = 0.0f;
    newViewport.MaxDepth = 1.0f;

    D3D12_RECT newRect = {};
    newRect.left = 0;
    newRect.top = 0;
    newRect.right = textureSize;
    newRect.bottom = textureSize;

    // Set command list params	
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = DX12Helper::GetInstance().GetCBVSRVDescriptorHeap();
    commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());
    commandList->SetGraphicsRootSignature(Assets::GetInstance().GetRootSig("iblIrradianceMapRS").Get());
    commandList->SetPipelineState(Assets::GetInstance().GetPipelineStateObject("iblIrradianceMapPSO").Get());
    commandList->RSSetViewports(1, &newViewport);
    commandList->RSSetScissorRects(1, &newRect);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    RtvSrvBundle irradianceMapRTVs[6] = {};

    for (int i = 0; i < 6; i++)
    {
        // Make a render target view for this face
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY; // This points to a Texture2D Array
        rtvDesc.Texture2DArray.ArraySize = 1;	// How much of the array do we need access to?
        rtvDesc.Texture2DArray.FirstArraySlice = i;	// Which texture are we rendering into?
        rtvDesc.Texture2DArray.MipSlice = 0;	// Which mip are we rendering into?
        rtvDesc.Format = texDesc.Format; // Same format as texture
        
        irradianceMapRTVs[i] = DX12Helper::GetInstance().CreateRtvSrvBundle(irrMapFinalTexture, rtvDesc, false);
        
        std::string rtvName = "irradianceMap" + std::to_string(i);
        Assets::GetInstance().AddRenderTargetView(rtvName, irradianceMapRTVs[i]);

        float color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        commandList->ClearRenderTargetView(irradianceMapRTVs[i].rtvHandle, color, 0, 0);
        commandList->OMSetRenderTargets(1, &irradianceMapRTVs[i].rtvHandle, true, &skyDsvHandle);

        IBLIrradianceMapData psData = {};
        psData.faceNum = i;

        D3D12_GPU_DESCRIPTOR_HANDLE cubeFaceHandle = DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&psData), sizeof(IBLIrradianceMapData));

        commandList->SetGraphicsRootDescriptorTable(0, cubeFaceHandle);
        commandList->SetGraphicsRootDescriptorTable(1, skyHandle);
        commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
    }
}

void Sky::IBLCreateConvolvedSpecularMap()
{
    RtvSrvBundle specMap = Assets::GetInstance().GetRenderTargetView("iblSpecularConvolution");
}

void Sky::IBLCreateBRDFLookupTable()
{
    Microsoft::WRL::ComPtr<ID3D12Resource> brdfTableTexture;
    // Create the final irradiance cube texture
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Width = cubeMapSize; 			// One of your constants
    texDesc.Height = cubeMapSize; 			// Same as width
    texDesc.DepthOrArraySize = 1; 					// Cube map means 6 textures
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // Will be used as both
    texDesc.Format = DXGI_FORMAT_R16G16_UNORM; 		// Basic texture format
    texDesc.MipLevels = 1; 					// No mip chain needed
    texDesc.SampleDesc.Count = 1; 				// Can't be zero
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    D3D12_HEAP_PROPERTIES heapDesc = {};
    heapDesc.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
    device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &texDesc, D3D12_RESOURCE_STATE_COMMON | D3D12_RESOURCE_STATE_RENDER_TARGET, 0, IID_PPV_ARGS(brdfTableTexture.GetAddressOf()));

    D3D12_VIEWPORT newViewport = {};
    newViewport.Width = textureSize;
    newViewport.Height = textureSize;
    newViewport.MinDepth = 0.0f;
    newViewport.MaxDepth = 1.0f;

    D3D12_RECT newRect = {};
    newRect.left = 0;
    newRect.top = 0;
    newRect.right = textureSize;
    newRect.bottom = textureSize;

    // Set command list params	
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = DX12Helper::GetInstance().GetCBVSRVDescriptorHeap();
    commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());
    commandList->SetGraphicsRootSignature(Assets::GetInstance().GetRootSig("iblBrdfRS").Get());
    commandList->SetPipelineState(Assets::GetInstance().GetPipelineStateObject("iblBrdfPSO").Get());
    commandList->RSSetViewports(1, &newViewport);
    commandList->RSSetScissorRects(1, &newRect);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // This points to a Texture2D Array
    rtvDesc.Texture2D.MipSlice = 0; // Which mip are we rendering into?
    rtvDesc.Format = texDesc.Format; // Same format as texture

    RtvSrvBundle brdfTable = DX12Helper::GetInstance().CreateRtvSrvBundle(brdfTableTexture, rtvDesc, false);
    Assets::GetInstance().AddRenderTargetView("iblBrdf", brdfTable);

    commandList->OMSetRenderTargets(1, &brdfTable.rtvHandle, true, &skyDsvHandle);
    commandList->DrawIndexedInstanced(3, 2, 0, 0, 0);
}
