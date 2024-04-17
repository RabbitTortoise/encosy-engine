module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Thesis.Systems.ThesisEntitySpawningSystemMt10;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import RenderCore.TextureLoader;
import RenderCore.MeshLoader;
import RenderCore.VulkanTypes;

import Components.TransformComponent;
import Components.MaterialComponent;
import Components.ModelMatrixComponent;
import Thesis.Components;

import <random>;
import <vector>;

export class ThesisEntitySpawningSystemMt10 : public SystemThreaded
{
	friend class SystemManager;

	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = true,
	.IgnoreThreadSaveFunctions = true,
	};

public:
	ThesisEntitySpawningSystemMt10(const int entityModificationsPerFrame, TextureLoader* textureLoader, MeshLoader* meshLoader, bool allowConcurrent) :
		MainTextureLoader(textureLoader), 
		MainMeshLoader(meshLoader),
		EntityModificationsPerFrame(entityModificationsPerFrame)
	{
		ThreadedRunOptions.PreferRunAlone = !allowConcurrent;
		std::random_device rd;
		std::mt19937 gen(rd());
		Gen = gen;
	}
	~ThesisEntitySpawningSystemMt10() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunAfterSpecificSystem = "ThesisEntityModifySystemMt10";
		RunWithSpecificSystem = "ThesisEntitySpawningSystemMt1";
		RunBeforeSpecificSystem = "ThesisEntityDestroySystemMt1";
		SetThreadedRunOptions(ThreadedRunOptions);

		SystemEntityType = GetEntityTypeInfo("EntityType10").Type;
		EnableDestructiveAccessToEntityStorage(SystemEntityType);
		AddSystemDataForReading(&SpawningSystemDataStorage);

		InitSpawningOptions();
	}

	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override 
	{
		const EntitySpawningSystemData spawnData = GetSystemData(&SpawningSystemDataStorage);

		TransformComponent transform;
		MaterialComponentLit material;
		ModelMatrixComponent modelMatrix = {};
		RotateComponent rotate;
		EntityTypeComponent10 entityType = {};

		for (size_t i = 0; i < EntityModificationsPerFrame; i++)
		{
			RandomizeComponentData(spawnData, transform, material, rotate);
			auto createdEntity = CreateEntityWithData(SystemEntityType, transform, material, modelMatrix, rotate, entityType);
		}
	}

	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override {}
	void PostUpdate(const int thread, const double deltaTime) override {}
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
			.Color = glm::vec3(1,1,1)
		};
		rotate =
		{
			.Direction = {rand1, rand2, rand3},
			.Speed = rand3 * 50.0f + 20.0f
		};
	}

	void InitSpawningOptions()
	{
		distr01 = std::uniform_real_distribution<float>(0.0f, 1.0f);

		auto grassID = MainTextureLoader->LoadTexture("Grass_Texture.png");
		auto grassNormalID = MainTextureLoader->LoadTexture("Grass_Normal.png");
		auto rockID = MainTextureLoader->LoadTexture("Rock_Texture.png");
		auto rockNormalID = MainTextureLoader->LoadTexture("Rock_Normal.png");
		auto sandID = MainTextureLoader->LoadTexture("Sand_Texture.png");
		auto sandNormalID = MainTextureLoader->LoadTexture("Sand_Normal.png");
		auto snowID = MainTextureLoader->LoadTexture("Snow_Texture.png");
		auto snowNormalID = MainTextureLoader->LoadTexture("Snow_Normal.png");
		auto waterID = MainTextureLoader->LoadTexture("Water_Texture.png");
		auto waterNormalID = MainTextureLoader->LoadTexture("Water_Normal.png");

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
