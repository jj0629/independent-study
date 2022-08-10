#pragma once

#include "GameEntity.h"
#include "Input.h"
#include "Assets.h"

// ImGui includes
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

class EngineGUI
{
public:
	EngineGUI(HWND hWnd, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	~EngineGUI();

	void Update(float deltaTime, float totalTime, unsigned int width, unsigned int height,
		std::vector<std::shared_ptr<GameEntity>> allEntities,
		std::vector<Light> lights);
	void Draw();

private:
	// Base info
	unsigned int width;
	unsigned int height;

	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	// Scene information
	std::vector<Light> lights;
	std::vector<std::shared_ptr<GameEntity>> allEntities;
	std::vector<std::shared_ptr<GameEntity>> standardEntities;
	std::vector<std::shared_ptr<GameEntity>> pbrEntities;
	std::vector<std::shared_ptr<GameEntity>> transparentEntities;
	std::vector<std::shared_ptr<GameEntity>> refractiveEntities;

	// Drawing methods (breaking up drawing for my own sanity)
	void CreateBaseTree(unsigned int width, unsigned int height);
	void DisplayGeneralInfo();
	void DisplayRTVImages();
	void DisplaySingleRTV(std::string name, RtvSrvBundle payload);
	void DisplayLights();
	void CreateSingleLight(Light* light, unsigned int lightNum);
	void DisplayGameEntities();
	void CreateSingleEntity(std::shared_ptr<GameEntity> ge, unsigned int geNum);
	void CreateSingleMaterial(std::string name, std::shared_ptr<Material> mat);
	void DisplayEmitters();
	//void CreateSingleEmitter(std::shared_ptr<Emitter> e, int eNum);
};

