#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Assets.h"

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true),			   // Show extra stats (fps) in title bar?
	vsync(false)
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

	ibView = {};
	vbView = {};

}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object created in Game

	delete &Assets::GetInstance();

	// We need to wait here until the GPU
	// is actually done with its work
	DX12Helper::GetInstance().WaitForGPU();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	Assets& am = Assets::GetInstance();
	am.Initialize("..\\..\\Assets\\", device, true, true);
	camera = std::make_shared<Camera>(0, 0, -10.0f, 5.0f, 1.0f, width / height);

	DX12Helper::GetInstance().SetWidth(width);
	DX12Helper::GetInstance().SetHeight(height);

	// Initialize the renderer
	renderer = std::make_shared<Renderer>(hWnd, device, commandList, swapChain, rtvHandles, dsvHandle, backBuffers, depthStencilBuffer, viewport, scissorRect, width, height);

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	CreateRootSigAndPipelineState();
	CreateBasicGeometry();
	CreateLights();
}

// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	// Create meshes
	std::shared_ptr<Mesh> cubeMesh = Assets::GetInstance().GetMesh("cube");
	std::shared_ptr<Mesh> sphereMesh = Assets::GetInstance().GetMesh("sphere");
	std::shared_ptr<Mesh> helixMesh = Assets::GetInstance().GetMesh("helix");

	// Create materials
	std::shared_ptr<Material> woodMat = Assets::GetInstance().GetMaterial("woodMat");
	std::shared_ptr<Material> scratchMat = Assets::GetInstance().GetMaterial("scratchMat");

	// Create game entities
	std::shared_ptr<GameEntity> cube1 = std::make_shared<GameEntity>(cubeMesh, woodMat);
	cube1->GetTransform()->SetPosition(5, 0, 0);
	entities.push_back(cube1);

	std::shared_ptr<GameEntity> cube2 = std::make_shared<GameEntity>(cubeMesh, scratchMat);
	cube2->GetTransform()->SetPosition(-5, 0, 0);
	entities.push_back(cube2);

	std::shared_ptr<GameEntity> sphere1 = std::make_shared<GameEntity>(sphereMesh, woodMat);
	sphere1->GetTransform()->SetPosition(3, 0, 0);
	entities.push_back(sphere1);

	std::shared_ptr<GameEntity> sphere2 = std::make_shared<GameEntity>(sphereMesh, scratchMat);
	sphere2->GetTransform()->SetPosition(-3, 0, 0);
	entities.push_back(sphere2);

	std::shared_ptr<GameEntity> helix1 = std::make_shared<GameEntity>(helixMesh, woodMat);
	helix1->GetTransform()->SetPosition(1, 0, 0);
	entities.push_back(helix1);

	std::shared_ptr<GameEntity> helix2 = std::make_shared<GameEntity>(helixMesh, scratchMat);
	helix2->GetTransform()->SetPosition(-1, 0, 0);
	entities.push_back(helix2);

	// Create the skybox now
	skyBox = std::make_shared<Sky>(this->device, this->commandList, this->dsvHandle, this->viewport, this->scissorRect, Assets::GetInstance().GetTexture("SkyBoxes\\SunnyCubeMap"), cubeMesh);
}

void Game::CreateLights()
{
	// Reset
	lights.clear();

	// Setup directional lights
	Light dir1 = {};
	dir1.Type = LIGHT_TYPE_DIRECTIONAL;
	dir1.Direction = XMFLOAT3(1, -1, 1);
	dir1.Color = XMFLOAT3(0.8f, 0.8f, 0.8f);
	dir1.Intensity = 1.0f;

	Light dir2 = {};
	dir2.Type = LIGHT_TYPE_DIRECTIONAL;
	dir2.Direction = XMFLOAT3(-1, -0.25f, 0);
	dir2.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir2.Intensity = 1.0f;

	Light dir3 = {};
	dir3.Type = LIGHT_TYPE_DIRECTIONAL;
	dir3.Direction = XMFLOAT3(0, -1, 1);
	dir3.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir3.Intensity = 1.0f;

	// Add light to the list
	lights.push_back(dir1);
	lights.push_back(dir2);
	lights.push_back(dir3);

	while (lights.size() < 10)
	{
		Light point = {};
		point.Type = LIGHT_TYPE_POINT;
		point.Position = XMFLOAT3(RandomRange(-10, 10), RandomRange(-2, 2), RandomRange(-10, 10));
		point.Color = XMFLOAT3(RandomRange(0, 1), RandomRange(0, 1), RandomRange(0, 1));
		point.Range = RandomRange(1, 10);
		point.Intensity = RandomRange(0.1f, 10);

		lights.push_back(point);
	}

	lightCount = lights.size();
}

// --------------------------------------------------------
// Loads the two basic shaders, then creates the root signature 
// and pipeline state object for our very basic demo.
// --------------------------------------------------------
void Game::CreateRootSigAndPipelineState()
{	
	//rootSignature = Assets::GetInstance().GetRootSig("pbrRS");
	//pipelineState = Assets::GetInstance().GetPipelineStateObject("basicPSO");

	RtvSrvBundle payload = Assets::GetInstance().GetRenderTargetView("test");
}

// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	renderer->PreResize();

	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// Update DX12Helper then the renderer
	DX12Helper::GetInstance().SetWidth(this->width);
	DX12Helper::GetInstance().SetHeight(this->width);

	renderer->PostResize(this->width, this->height, backBuffers, depthStencilBuffer, rtvHandles, dsvHandle, viewport, scissorRect);

	if (camera) {
		camera->UpdateProjectionMatrix((float)width / height);
	}
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	// Spin entities
	for (auto& e : entities)
	{
		e->GetTransform()->Rotate(0, deltaTime * 0.5f, 0);
	}

	camera->Update(deltaTime);

	// Update the entities in the renderer
	renderer->Update(deltaTime, totalTime, entities, skyBox, lights, lightCount);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	renderer->Render(camera, deltaTime, totalTime);
}