module;
#include <glm/glm.hpp>	

export module Thesis.Systems.ThesisEntitySpawningSystemSt5;

import EncosyCore.Entity;
import EncosyCore.System;
import RenderCore.TextureLoader;
import RenderCore.MeshLoader;
import RenderCore.VulkanTypes;

import Components.TransformComponent;
import Components.MaterialComponent;
import Components.ModelMatrixComponent;
import Thesis.Components;

import <random>;
import <vector>;

export class ThesisEntitySpawningSystemSt5 : public System
{
	friend class SystemManager;

public:
	ThesisEntitySpawningSystemSt5(const int entityModificationsPerFrame, TextureLoader* textureLoader, MeshLoader* meshLoader) :
		MainTextureLoader(textureLoader), 
		MainMeshLoader(meshLoader),
		EntityModificationsPerFrame(entityModificationsPerFrame)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		Gen = gen;
	}
	~ThesisEntitySpawningSystemSt5() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunAfterSpecificSystem = "ThesisEntityModifySystemSt10";
		RunWithSpecificSystem = "ThesisEntitySpawningSystemSt1";
		RunBeforeSpecificSystem = "ThesisEntityDestroySystemSt1";

		SystemEntityType = GetEntityTypeInfo("EntityType5").Type;
		EnableDestructiveAccessToEntityStorage(SystemEntityType);
		AddSystemDataForReading(&SpawningSystemDataStorage);

		InitSpawningOptions();
	}

	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override 
	{
		const EntitySpawningSystemData spawnData = GetSystemData(&SpawningSystemDataStorage);

		TransformComponent transform;
		MaterialComponentLit material;
		ModelMatrixComponent modelMatrix = {};
		RotateComponent rotate;
		EntityTypeComponent5 entityType = {};

		for (size_t i = 0; i < EntityModificationsPerFrame; i++)
		{
			RandomizeComponentData(spawnData, transform, material, rotate);
			auto createdEntity = CreateEntityWithData(SystemEntityType, transform, material, modelMatrix, rotate, entityType);
		}
	}
	void UpdatePerEntity(const double deltaTime, const Entity entity, const EntityType entityType) override {}
	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

	void RandomizeComponentData(
		const EntitySpawningSystemData& spawnData,
		TransformComponent& transform,
		MaterialComponentLit& material,
		RotateComponent &rotate)
	{
		auto distrX = std::uniform_real_distribution<float>(spawnData.SpawnRegionMin.x, spawnData.SpawnRegionMax.x);
		auto distrY = std::uniform_real_distribution<float>(spawnData.SpawnRegionMin.y, spawnData.SpawnRegionMax.y);
		auto distrZ = std::uniform_real_distribution<float>(spawnData.SpawnRegionMin.z, spawnData.SpawnRegionMax.z);

		const glm::vec3 randPos = glm::vec3(distrX(Gen), distrY(Gen), distrZ(Gen));
		float rand1 = distr01(Gen);
		float rand2 = distr01(Gen);
		float rand3 = distr01(Gen);
		const int textureSelect = std::round(rand1 * (textureIds.size() - 1));
		const int meshSelect = std::round(rand2 * (meshIDs.size() - 1));

		transform =
		{
			.Position = randPos,
			.Scale = {1,1,1},
			.Orientation = glm::quat(glm::vec3(0,0,0))
		};

		const TextureID usedTextureID = textureIds[textureSelect];
		const MeshID usedMeshId = meshIDs[meshSelect];
		material =
		{
			.Diffuse = usedTextureID,
			.Normal = usedTextureID + 1,
			.RenderMesh = usedMeshId,
			.TextureRepeat = 1.0f,
			.Color = glm::vec3(1,1,1) };
		rotate =
		{
			.Direction = {rand1, rand2, rand3},
			.Speed = rand3 * 500.0f + 500.0f
		};
	}

	void InitSpawningOptions()
	{
		distr01 = std::uniform_real_distribution<float>(0.0f, 1.0f);

		const auto grassID = MainTextureLoader->LoadTexture("Grass_Texture.png");
		const auto grassNormalID = MainTextureLoader->LoadTexture("Grass_Normal.png");
		const auto rockID = MainTextureLoader->LoadTexture("Rock_Texture.png");
		const auto rockNormalID = MainTextureLoader->LoadTexture("Rock_Normal.png");
		const auto sandID = MainTextureLoader->LoadTexture("Sand_Texture.png");
		const auto sandNormalID = MainTextureLoader->LoadTexture("Sand_Normal.png");
		const auto snowID = MainTextureLoader->LoadTexture("Snow_Texture.png");
		const auto snowNormalID = MainTextureLoader->LoadTexture("Snow_Normal.png");
		const auto waterID = MainTextureLoader->LoadTexture("Water_Texture.png");
		const auto waterNormalID = MainTextureLoader->LoadTexture("Water_Normal.png");

		textureIds.push_back(grassID);
		textureIds.push_back(rockID);
		textureIds.push_back(sandID);
		textureIds.push_back(snowID);
		textureIds.push_back(waterID);

		meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Cube));
		meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere));
		meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Torus));
	}

private:
	EntityType SystemEntityType = -1;
	int EntityModificationsPerFrame;

	ReadOnlySystemDataStorage<EntitySpawningSystemData> SpawningSystemDataStorage;

	std::mt19937 Gen;
	std::uniform_real_distribution<float> distr01;
	
	std::vector<TextureID> textureIds;
	std::vector<MeshID> meshIDs;

	TextureLoader* MainTextureLoader;
	MeshLoader* MainMeshLoader;
};
