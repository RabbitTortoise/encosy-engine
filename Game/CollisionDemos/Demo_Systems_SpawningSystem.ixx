module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo_Systems_SpawningSystem;

import EE_Encosy_Entity;
import EE_Encosy_SystemThreaded;
import EE_RenderCore_TextureLoader;
import EE_RenderCore_MeshLoader;
import EE_RenderCore_VulkanTypes;
import Demo_SystemData_SpawningSystem;

import EE_ECS_Components_TransformComponent;
import EE_ECS_Components_MaterialComponent;
import Demo_Components_SphereColliderComponent;
import Demo_Components_FollowerComponent;
import Demo_Components_MovementComponent;
import Demo_Components_FollowerComponent;
import Demo_Components_LeaderComponent;

import <map>;
import <span>;
import <vector>;
import <chrono>;
import <thread>;
import <random>;
import <numeric>;


export class SpawningSystem : public SystemThreaded
{
	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = true,
	.IgnoreThreadSaveFunctions = true,
	};

public:
	SpawningSystem(TextureLoader* textureLoader, MeshLoader* meshLoader) : MainTextureLoader(textureLoader), MainMeshLoader(meshLoader) {}
	~SpawningSystem() {}

	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		SetThreadedRunOptions(ThreadedRunOptions);

		FollowerType = GetEntityTypeInfo("FollowerEntity").Type;
		LeaderType = GetEntityTypeInfo("LeaderEntity").Type;

		EnableDestructiveAccessToEntityStorage(GetEntityTypeInfo("FollowerEntity").Type);
		AddSystemDataForReading(&SpawningSystemDataComponent);
		AddComponentQueryForReading(&TransformComponents);
		AddComponentQueryForReading(&LeaderComponents);
		AddComponentQueryForReading(&MaterialComponents);

		std::random_device rd;
		std::mt19937 gen(rd());
		Gen = gen;


	}
	void PreUpdate(const int thread, const double deltaTime) override{}

	void Update(const int thread, const double deltaTime) override
	{
		auto systemData = GetSystemData(&SpawningSystemDataComponent);
		LastFrametimes[CurrentFrameTimeIndex] = deltaTime;
		CurrentFrameTimeIndex++;
		if (CurrentFrameTimeIndex == LastFrametimes.size()) { CurrentFrameTimeIndex = 0; }
		float averageFrametime = std::accumulate(LastFrametimes.begin(), LastFrametimes.end(), 0.0) / LastFrametimes.size();
		averageFPS = 1.0 / averageFrametime;
		LastSpawnTime += deltaTime;
		if (LastSpawnTime > 1.0f) { LastSpawnTime = 1.0f; }
		if (LastSpawnTime - SpawnInterval > 0.0f) { SpawnTimeLeft = true; } else { SpawnTimeLeft = false; }

		if (averageFPS < systemData.minFPS) { SpawningEnabled = false; }
		if (!SpawningEnabled)
		{
			if (averageFPS > systemData.resetFPS) { SpawningEnabled = true; }
		}
	}

	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		std::uniform_real_distribution<float> distr(0.0f, 1.0f);
		float randomValue = distr(Gen);
		if (randomValue < 0.1 && SpawnTimeLeft && SpawningEnabled)
		{
			auto systemData = GetSystemData(&SpawningSystemDataComponent);
			
			EE_TransformComponent ltc = GetCurrentEntityComponent(thread, &TransformComponents);
			LeaderComponent llc = GetCurrentEntityComponent(thread, &LeaderComponents);
			EE_MaterialComponent mcRay = GetCurrentEntityComponent(thread, &MaterialComponents);

			std::uniform_real_distribution<float> distrX(systemData.PlayRegionMin.x, systemData.PlayRegionMax.x);
			std::uniform_real_distribution<float> distrY(systemData.PlayRegionMin.y, systemData.PlayRegionMax.y);
			std::uniform_real_distribution<float> distrZ(systemData.PlayRegionMin.z, systemData.PlayRegionMax.z);

			glm::vec3 randPos = glm::vec3(distrX(Gen), distrY(Gen),distrZ(Gen));
			glm::vec3 direction = randPos - ltc.Position;

			EE_TransformComponent newtc = ltc;
			newtc.Position.x += direction.x / 10.0f;
			newtc.Position.y += direction.y / 10.0f;
			newtc.Position.z += direction.z / 10.0f;
			newtc.Scale = glm::vec3(Scale, Scale, Scale);

			FollowerComponent newfc = { llc.LeaderID };
			MovementComponent newmov = { {}, {Speed} };
			SphereColliderComponent newsphere = SphereColliderComponent(CollisionRadius, false);

			mcRay.TextureSet = llc.LeaderID;
			mcRay.RenderMesh = MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere);
			mcRay.TextureRepeat = mcRay.TextureRepeat;
			mcRay.Color = glm::vec3(1, 1, 1);

			CreateEntityWithData(FollowerType, newtc, mcRay, newfc, newmov, newsphere);
		}
	}

	void PostUpdate(const int thread, const double deltaTime) override
	{
		if (SpawnTimeLeft)
		{
			LastSpawnTime -= SpawnInterval;
		}
	}
	void Destroy() override {}

private:

	ReadOnlySystemDataStorage<SpawningSystemData> SpawningSystemDataComponent; 

	ReadOnlyComponentStorage<EE_TransformComponent> TransformComponents;
	ReadOnlyComponentStorage<LeaderComponent> LeaderComponents;
	ReadOnlyComponentStorage<EE_MaterialComponent> MaterialComponents;

	EntityType LeaderType;
	EntityType FollowerType;

	bool SpawningEnabled = true;
	bool SpawnTimeLeft = true;

	float Speed = 6.0f;
	float Scale = 3.0f;
	float CollisionRadius = 0.51f;
	float SpawnInterval = 0.02f;
	float LastSpawnTime = 0.0f;
	double averageFPS = 0.0;

	std::mt19937 Gen;

	std::vector<TextureID> textureIds;
	std::vector<MeshID> meshIDs;

	TextureLoader* MainTextureLoader;
	MeshLoader* MainMeshLoader;

	std::vector<double> LastFrametimes = std::vector<double>(60, 0.0001);
	int CurrentFrameTimeIndex = 0;
};
