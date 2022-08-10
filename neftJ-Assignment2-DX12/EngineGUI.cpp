#include "EngineGUI.h"

EngineGUI::EngineGUI(HWND hWnd, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = DX12Helper::GetInstance().GetCBVSRVDescriptorHeap();
	this->device = device;
	this->commandList = commandList;
	D3D12_HANDLE_BUNDLE fontHandles = DX12Helper::GetInstance().GetNextCBVSRVHeapLocationHandles();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX12_Init(device.Get(), 2,
		DXGI_FORMAT_R8G8B8A8_UNORM, descriptorHeap.Get(),
        fontHandles.cpuHandle,
		fontHandles.gpuHandle);
}

EngineGUI::~EngineGUI()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void EngineGUI::Update(float deltaTime, float totalTime, unsigned int width, unsigned int height, std::vector<std::shared_ptr<GameEntity>> allEntities, std::vector<Light> lights)
{
	// Update engineGUI elements
	this->width = width;
	this->height = height;
	this->allEntities = allEntities;
	this->lights = lights;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	// Update ImGui properties
	Input& input = Input::GetInstance();

	input.SetGuiKeyboardCapture(false);
	input.SetGuiMouseCapture(false);

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)width;
	io.DisplaySize.y = (float)height;
	io.KeyCtrl = input.KeyDown(VK_CONTROL);
	io.KeyShift = input.KeyDown(VK_SHIFT);
	io.KeyAlt = input.KeyDown(VK_MENU);
	io.MousePos.x = (float)input.GetMouseX();
	io.MousePos.y = (float)input.GetMouseY();
	io.MouseDown[0] = input.MouseLeftDown();
	io.MouseDown[1] = input.MouseRightDown();
	io.MouseDown[2] = input.MouseMiddleDown();
	io.MouseWheel = input.GetMouseWheel();
	input.GetKeyArray(io.KeysDown, 256);

	input.SetGuiKeyboardCapture(io.WantCaptureKeyboard);
	input.SetGuiMouseCapture(io.WantCaptureMouse);
    
	CreateBaseTree(width, height);
}

void EngineGUI::Draw()
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

void EngineGUI::CreateBaseTree(unsigned int width, unsigned int height)
{
	ImGui::Begin("Justin's Window");

	DisplayGeneralInfo();
	DisplayRTVImages();
	DisplayLights();
	DisplayGameEntities();
	DisplayEmitters();

	ImGui::End();
}

void EngineGUI::DisplayGeneralInfo()
{
	ImGuiIO& io = ImGui::GetIO();
	if (ImGui::CollapsingHeader("App Info"))
	{
		ImGui::Text(("FPS: " + std::to_string(io.Framerate)).c_str());
		ImGui::Text(("Width: " + std::to_string(width) + " Height: " + std::to_string(height)).c_str());
		ImGui::Text(("Aspect Ratio: " + std::to_string((float)width / (float)height)).c_str());
	}
}

void EngineGUI::DisplayRTVImages()
{
	if (ImGui::CollapsingHeader("RTV Images"))
	{
		std::unordered_map<std::string, RtvSrvBundle> rtvSrvBundles = Assets::GetInstance().GetAllRTVs();
		for (const auto& [key, value] : rtvSrvBundles) {
			DisplaySingleRTV(key, value);
		}
	}
}

void EngineGUI::DisplaySingleRTV(std::string name, RtvSrvBundle payload)
{
	if (ImGui::CollapsingHeader(name.c_str()))
	{
		ImVec2 imageSize = ImGui::GetWindowSize();
		float imageHeight = imageSize.x * ((float)height / width);
		ImGui::Image((ImTextureID)payload.srvGPUHandle.ptr, ImVec2(imageSize.x, imageHeight));
	}
}

void EngineGUI::DisplayLights()
{
}

void EngineGUI::DisplayGameEntities()
{
}

void EngineGUI::DisplayEmitters()
{
}
