#include "Renderer.h"

Renderer::Renderer(
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
	unsigned int height) :
	dsvHandle(dsvHandle),
	depthStencilBuffer(depthStencilBuffer)
{
	this->device = device;
	this->commandList = commandList;
	this->swapChain = swapChain;
	for (int i = 0; i < numBackBuffers; i++)
	{
		this->rtvHandles[i] = rtvHandles[i];
	}

	for (int i = 0; i < numBackBuffers; i++)
	{
		this->backBuffers[i] = backBuffers[i];
	}
	this->viewport = viewport;
	this->scissorRect = scissorRect;
	
	this->width = width;
	this->height = height;
	this->currentSwapBuffer = 0;

	// Create ImGUI interface
	engineGUI = std::make_shared<EngineGUI>(hWnd, device, commandList);
}

Renderer::~Renderer()
{

}

void Renderer::PreResize()
{
	for (int i = 0; i < numBackBuffers; i++)
	{
		backBuffers[i].Reset();
	}
	depthStencilBuffer.Reset();
	Assets::GetInstance().ReleaseRTVs();
}

void Renderer::PostResize(unsigned int windowWidth, unsigned int windowHeight, Microsoft::WRL::ComPtr<ID3D12Resource> backBuffers[], Microsoft::WRL::ComPtr<ID3D12Resource> depthBufferDSV, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[], D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect)
{
	currentSwapBuffer = 0;

	this->depthStencilBuffer = depthBufferDSV;
	for (int i = 0; i < numBackBuffers; i++)
	{
		this->backBuffers[i] = backBuffers[i];
	}

	for (int i = 0; i < numBackBuffers; i++)
	{
		this->rtvHandles[i] = rtvHandles[i];
	}
	this->dsvHandle = dsvHandle;
	this->viewport = viewport;
	this->scissorRect = scissorRect;

	DX12Helper::GetInstance().SetWidth(windowWidth);
	DX12Helper::GetInstance().SetHeight(windowHeight);

	Assets::GetInstance().ReloadAllRTVs();
}

void Renderer::Update(float deltaTime, float totalTime, std::vector<std::shared_ptr<GameEntity>> entities, std::shared_ptr<Sky> skyBox, std::vector<Light> lights, int lightCount)
{
	this->allEntities = entities;
	this->skyBox = skyBox;
	this->lights = lights;
	this->lightCount = lightCount;

	this->SortEntityVectors();

	engineGUI->Update(deltaTime, totalTime, width, height, allEntities, lights);
}

void Renderer::Render(std::shared_ptr<Camera> camera, float deltaTime, float totalTime)
{
	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];

	// Clearing the render target
	{
		// Transition the back buffer from present to render target
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Background color (Cornflower Blue in this case) for clearing
		float color[] = { 0.4f, 0.6f, 0.75f, 1.0f };

		// Clear the RTV
		commandList->ClearRenderTargetView(
			rtvHandles[currentSwapBuffer],
			color,
			0, 0); // No scissor rectangles

		// Clear the depth buffer, too
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,	// Max depth = 1.0f
			0,		// Not clearing stencil, but need a value
			0, 0);	// No scissor rects
	}

	ClearRenderTargets();

	// Do our engine rendering passes here
	{
		if (skyBox->GetIBLCreationState() == false) skyBox->CreateIBLResources();
		StandardEntities(camera, deltaTime, totalTime);
		PbrEntities(camera, deltaTime, totalTime);
		skyBox->Draw(camera);
		TransparentEntities(camera, deltaTime, totalTime);
		RefractiveEntities(camera, deltaTime, totalTime);
		Emitters(camera, deltaTime, totalTime);
		DepthOfField(camera, deltaTime, totalTime);
		FinalTextureToScreen(camera, deltaTime, totalTime);
		engineGUI->Draw();
	}

	// Present
	{
		// Transition back to present
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);
		
		// Must occur BEFORE present
		DX12Helper::GetInstance().CloseExecuteAndResetCommandList();

		// Present the current back buffer
		swapChain->Present(vsync ? 1 : 0, 0);

		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;
	}
}

void Renderer::SortEntityVectors()
{
	// Clear all of our entity vectors first to avoid any duplicates
	standardEntities.clear();
	pbrEntities.clear();
	transparentEntities.clear();
	refractiveEntities.clear();

	for (auto& e : allEntities)
	{
		// Get the pso from the entitiy and check it to see what type of object it is
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso = e->GetMaterial()->GetPipelineState();

		if (pso == Assets::GetInstance().GetPipelineStateObject("basicPSO"))
		{
			standardEntities.push_back(e);
		}
		else if (pso == Assets::GetInstance().GetPipelineStateObject("pbrPSO"))
		{
			pbrEntities.push_back(e);
		}
		else if (pso == Assets::GetInstance().GetPipelineStateObject("transparentPSO"))
		{
			transparentEntities.push_back(e);
		}
		else if (pso == Assets::GetInstance().GetPipelineStateObject("refractivePSO"))
		{
			refractiveEntities.push_back(e);
		}
	}
}

void Renderer::ClearRenderTargets()
{
	std::unordered_map<std::string, RtvSrvBundle> rtvSrvBundles = Assets::GetInstance().GetAllRTVs();
	float color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for (const auto& [key, value] : rtvSrvBundles) {
		commandList->ClearRenderTargetView(value.rtvHandle, color, 0, 0);
	}
}

void Renderer::StandardEntities(std::shared_ptr<Camera> camera, float deltaTime, float totalTime)
{
	for (auto& e : standardEntities)
	{
		std::shared_ptr<Material> mat = e->GetMaterial();

		// Check if it's a new root sig being put in
		if (currentRootSig.GetAddressOf() != mat->GetRootSig().GetAddressOf())
		{
			commandList->SetGraphicsRootSignature(mat->GetRootSig().Get());
			currentRootSig = mat->GetRootSig();
		}

		// Set descriptor heap		
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = DX12Helper::GetInstance().GetCBVSRVDescriptorHeap();
		commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

		// Set up other commands for rendering
		RtvSrvBundle firstCompositeBundle = Assets::GetInstance().GetRenderTargetView("firstComposite");
		commandList->OMSetRenderTargets(1, &firstCompositeBundle.rtvHandle, true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (currentPSO.GetAddressOf() != mat->GetPipelineState().GetAddressOf())
		{
			commandList->SetPipelineState(mat->GetPipelineState().Get());
			currentPSO = mat->GetPipelineState();
		}

		VertexShaderExternalData vsData = {};
		vsData.World = e->GetTransform()->GetWorldMatrix();
		vsData.WorldInverseTranspose = e->GetTransform()->GetWorldInverseTransposeMatrix();
		vsData.View = camera->GetView();
		vsData.Projection = camera->GetProjection();

		D3D12_GPU_DESCRIPTOR_HANDLE cbHandleVS = DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&vsData), sizeof(VertexShaderExternalData));
		commandList->SetGraphicsRootDescriptorTable(0, cbHandleVS);

		PixelShaderExternalData psData = {};
		psData.cameraPosition = camera->GetTransform()->GetPosition();
		psData.uvScale = mat->GetUVScale();
		psData.uvOffset = mat->GetUVOffset();
		psData.lightCount = lights.size();
		memcpy(psData.lights, &lights[0], sizeof(Light) * lights.size());

		D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePS = DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&psData), sizeof(PixelShaderExternalData));
		commandList->SetGraphicsRootDescriptorTable(1, cbHandlePS);

		commandList->SetGraphicsRootDescriptorTable(2, mat->GetFinalGPUHandleForSRVs());

		std::shared_ptr<Mesh> mesh = e->GetMesh();
		D3D12_VERTEX_BUFFER_VIEW vbv = mesh->GetVertexBuffer();
		D3D12_INDEX_BUFFER_VIEW ibv = mesh->GetIndexBuffer();

		commandList->IASetVertexBuffers(0, 1, &vbv);
		commandList->IASetIndexBuffer(&ibv);

		commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
	}
}

void Renderer::PbrEntities(std::shared_ptr<Camera> camera, float deltaTime, float totalTime)
{
	PbrPsPerFrame psFrameData = {};
	psFrameData.cameraPosition = camera->GetTransform()->GetPosition();
	psFrameData.lightCount = lights.size();
	memcpy(psFrameData.lights, &lights[0], sizeof(Light) * lights.size());

	D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePsFrame = DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&psFrameData), sizeof(PbrPsPerFrame));

	// Grab our RTVs
	RtvSrvBundle colorNoAmbientBundle = Assets::GetInstance().GetRenderTargetView("colorNoAmbient");
	RtvSrvBundle ambientColorBundle = Assets::GetInstance().GetRenderTargetView("ambientColor");
	RtvSrvBundle normalsBundle = Assets::GetInstance().GetRenderTargetView("normals");
	RtvSrvBundle depthsBundle = Assets::GetInstance().GetRenderTargetView("depths");

	// Set them
	D3D12_CPU_DESCRIPTOR_HANDLE targets[4] = {};
	targets[0] = colorNoAmbientBundle.rtvHandle;
	targets[1] = ambientColorBundle.rtvHandle;
	targets[2] = normalsBundle.rtvHandle;
	targets[3] = depthsBundle.rtvHandle;

	commandList->OMSetRenderTargets(4, &targets[0], true, &dsvHandle);

	for (auto& e : pbrEntities)
	{
		std::shared_ptr<Material> mat = e->GetMaterial();

		// Check if it's a new root sig being put in
		if (currentRootSig.GetAddressOf() != mat->GetRootSig().GetAddressOf())
		{
			commandList->SetGraphicsRootSignature(mat->GetRootSig().Get());
			currentRootSig = mat->GetRootSig();
		}

		// Set descriptor heap		
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = DX12Helper::GetInstance().GetCBVSRVDescriptorHeap();
		commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

		// Set up other commands for rendering
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (currentPSO.GetAddressOf() != mat->GetPipelineState().GetAddressOf())
		{
			commandList->SetPipelineState(mat->GetPipelineState().Get());
			currentPSO = mat->GetPipelineState();
		}

		VertexShaderExternalData vsData = {};
		vsData.World = e->GetTransform()->GetWorldMatrix();
		vsData.WorldInverseTranspose = e->GetTransform()->GetWorldInverseTransposeMatrix();
		vsData.View = camera->GetView();
		vsData.Projection = camera->GetProjection();

		D3D12_GPU_DESCRIPTOR_HANDLE cbHandleVS = DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&vsData), sizeof(VertexShaderExternalData));
		commandList->SetGraphicsRootDescriptorTable(0, cbHandleVS);

		commandList->SetGraphicsRootDescriptorTable(1, cbHandlePsFrame);

		PbrPsPerMaterial psMatData = {};
		psMatData.colorTint = mat->GetColorTint();
		psMatData.uvOffset = mat->GetUVOffset();
		psMatData.uvScale = mat->GetUVScale();

		D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePsMat = DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&psMatData), sizeof(PbrPsPerMaterial));
		commandList->SetGraphicsRootDescriptorTable(2, cbHandlePsMat);

		commandList->SetGraphicsRootDescriptorTable(3, mat->GetFinalGPUHandleForSRVs());

		std::shared_ptr<Mesh> mesh = e->GetMesh();
		D3D12_VERTEX_BUFFER_VIEW vbv = mesh->GetVertexBuffer();
		D3D12_INDEX_BUFFER_VIEW ibv = mesh->GetIndexBuffer();

		commandList->IASetVertexBuffers(0, 1, &vbv);
		commandList->IASetIndexBuffer(&ibv);

		commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
	}
}

void Renderer::TransparentEntities(std::shared_ptr<Camera> camera, float deltaTime, float totalTime)
{
	
}

void Renderer::RefractiveEntities(std::shared_ptr<Camera> camera, float deltaTime, float totalTime)
{
}

void Renderer::Emitters(std::shared_ptr<Camera> camera, float deltaTime, float totalTime)
{
}

void Renderer::DepthOfField(std::shared_ptr<Camera> camera, float deltaTime, float totalTime)
{
}

void Renderer::FinalTextureToScreen(std::shared_ptr<Camera> camera, float deltaTime, float totalTime)
{
	// Set descriptor heap		
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = DX12Helper::GetInstance().GetCBVSRVDescriptorHeap();
	commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

	commandList->SetGraphicsRootSignature(Assets::GetInstance().GetRootSig("fullscreenRS").Get());

	// Set up other commands for rendering
	commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetPipelineState(Assets::GetInstance().GetPipelineStateObject("fullscreenPSO").Get());
	RtvSrvBundle colorNoAmbientBundle = Assets::GetInstance().GetRenderTargetView("colorNoAmbient");
	commandList->SetGraphicsRootDescriptorTable(0, colorNoAmbientBundle.srvGPUHandle);

	commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
}
