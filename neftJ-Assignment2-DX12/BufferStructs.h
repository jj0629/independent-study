#pragma once
#include <DirectXMath.h>
using namespace DirectX;

#include "Lights.h"

struct VertexShaderExternalData 
{
	XMFLOAT4X4 World;
	XMFLOAT4X4 WorldInverseTranspose;
	XMFLOAT4X4 View;
	XMFLOAT4X4 Projection;
};

// Alignment matters!!!
struct PixelShaderExternalData
{
	XMFLOAT2 uvScale;
	XMFLOAT2 uvOffset;
	XMFLOAT3 cameraPosition;
	//XMFLOAT3 colorTint;
	int lightCount;
	Light lights[MAX_LIGHTS];
};

struct PbrPsPerMaterial
{
	// Surface color
	XMFLOAT3 colorTint;
	float padding;

	// UV adjustments
	XMFLOAT2 uvScale;

	XMFLOAT2 uvOffset;
};

struct PbrPsPerFrame
{
	// An array of light data
	Light lights[MAX_LIGHTS];

	// The amount of lights THIS FRAME
	int lightCount;

	XMFLOAT3 cameraPosition;

	// Needed for IBL calculations
	//int SpecIBLTotalMipLevels;
};

struct SkyVSData
{
	XMFLOAT4X4 View;
	XMFLOAT4X4 Projection;
};

struct IBLIrradianceMapData
{
	int faceNum;
	XMFLOAT3 padding;
};

struct IBLSpecularConvolutionData
{
	float roughness;
	int faceIndex;
	int mipLevel;
	float padding;
};