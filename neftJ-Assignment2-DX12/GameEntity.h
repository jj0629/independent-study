#pragma once
#include "Mesh.h"
#include "Transform.h"
#include "Material.h"

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> m, std::shared_ptr<Material> mat);
	~GameEntity();

	std::shared_ptr<Mesh> GetMesh();
	Transform* GetTransform();
	std::shared_ptr<Material> GetMaterial();

	void SetMesh(std::shared_ptr<Mesh> m);
	void SetMaterial(std::shared_ptr<Material> mat);

private:
	std::shared_ptr<Mesh> mesh;
	Transform transform;
	std::shared_ptr<Material> material;
};

