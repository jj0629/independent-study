#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "Vertex.h"
#include "DX12Helper.h"


class Mesh
{
public:
	Mesh(Vertex* vertArray, int numVerts, unsigned int* indexArray, int numIndices);
	Mesh(const char* objFile);
	~Mesh();

	D3D12_VERTEX_BUFFER_VIEW GetVertexBuffer() { return vbView; }
	D3D12_INDEX_BUFFER_VIEW GetIndexBuffer() { return ibView; }
	int GetIndexCount() { return numIndices; }

private:
	D3D12_VERTEX_BUFFER_VIEW vbView;
	Microsoft::WRL::ComPtr<ID3D12Resource> vb;
	D3D12_INDEX_BUFFER_VIEW ibView;
	Microsoft::WRL::ComPtr<ID3D12Resource> ib;
	int numIndices;

	void CreateBuffers(Vertex* vertArray, int numVerts, unsigned int* indexArray, int numIndices);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
};

