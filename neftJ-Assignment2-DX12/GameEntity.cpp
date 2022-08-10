#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> m, std::shared_ptr<Material> mat)
{
    this->mesh = m;
    this->material = mat;
    transform = Transform();
}

GameEntity::~GameEntity()
{
    
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
    return this->mesh;
}

Transform* GameEntity::GetTransform()
{
    return &transform;
}

std::shared_ptr<Material> GameEntity::GetMaterial()
{
    return this->material;
}

void GameEntity::SetMesh(std::shared_ptr<Mesh> m)
{
    this->mesh = m;
}

void GameEntity::SetMaterial(std::shared_ptr<Material> mat)
{
    this->material = mat;
}
