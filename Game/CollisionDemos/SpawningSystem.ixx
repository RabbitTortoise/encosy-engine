module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo.Systems.SpawningSystem;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import RenderCore.TextureLoader;
import RenderCore.MeshLoader;
import RenderCore.VulkanTypes;
import Demo.SystemData.SpawningSystem;

import Components.TransformComponent;
import Components.MaterialComponent;
import Components.ModelMatrixComponent;
import Demo.Components.SphereColliderComponent;
import Demo.Components.FollowerComponent;
import Demo.Components.MovementComponent;
import Demo.Components.FollowerComponent;
import Demo.Components.LeaderComponent;

import <map>;
import <span>;
import <vector>;
import <chrono>;
import <thread>;
import <random>;
import <numeric>;


export class SpawningSystem : public SystemThreaded
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
	SpawningSystem(TextureLoader* textureLoader, MeshLoader* meshLoader) : MainTextureLoader(textureLoader), MainMeshLoader(meshLoader) {}
	~SpawningSystem() {}

protected:
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
		LastFrametimes[CurrentFrameTimeIndex] = deltaTime;
		CurrentFrameTimeIndex++;
		if (CurrentFrameTimeIndex == LastFrametimes.size()) { CurrentFrameTimeIndex = 0; }
		float averageFrametime = std::accumulate(LastFrametimes.begin(), LastFrametimes.end(), 0.0) / LastFrametimes.size();
		averageFPS = 1.0 / averageFrametime;
		LastSpawnTime += deltaTime;
		if (LastSpawnTime > 1.0f) { LastSpawnTime = 1.0f; }
		if (LastSpawnTime - SpawnInterval > 0.0f) { SpawnTimeLeft = true; } else { SpawnTimeLeft = false; }

		if (averageFPS < 100.0f) { SpawningEnabled = false; }
		if (!SpawningEnabled)
		{
			if (averageFPS > 110.0f) { SpawningEnabled = true; }
		}
	}

	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		std::uniform_real_distribution<float> distr(0.0f, 1.0f);
		float randomValue = distr(Gen);
		if (randomValue < 0.1 && SpawnTimeLeft && SpawningEnabled)
		{
			auto systemData = GetSystemData(&SpawningSystemDataComponent);
			
			TransformComponent ltc = GetCurrentEntityComponent(thread, &TransformComponents);
			LeaderComponent llc = GetCurrentEntityComponent(thread, &LeaderComponents);
			MaterialComponentLit lmc = GetCurrentEntityComponent(thread, &MaterialComponents);

			std::uniform_real_distribution<float> distrX(systemData.PlayRegionMin.x, systemData.PlayRegionMax.x);
			std::uniform_real_distribution<float> distrY(systemData.PlayRegionMin.y, systemData.PlayRegionMax.y);
			std::uniform_real_distribution<float> distrZ(systemData.PlayRegionMin.z, systemData.PlayRegionMax.z);

			glm::vec3 randPos = glm::vec3(distrX(Gen), distrY(Gen),distrZ(Gen));
			glm::vec3 direction = randPos - ltc.Position;

			TransformComponent newtc = ltc;
			newtc.Position.x += direction.x / 10.0f;
			newtc.Position.y += direction.y / 10.0f;
			newtc.Position.z += direction.z / 10.0f;
			newtc.Scale = glm::vec3(Scale, Scale, Scale);

			MaterialComponentLit newmat = { lmc.Diffuse, lmc.Normal, MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere), lmc.TextureRepeat };
			ModelMatrixComponent newmatrix = {};
			FollowerComponent newfc = { llc.LeaderID };
			MovementComponent newmov = { {}, {Speed} };
			SphereColliderComponent newsphere = { CollisionRadius };
			CreateEntityWithData(FollowerType, newtc, newmat, newmatrix, newfc, newmov, newsphere);
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

	ReadOnlyComponentStorage<TransformComponent> TransformComponents;
	ReadOnlyComponentStorage<LeaderComponent> LeaderComponents;
	ReadOnlyComponentStorage<MaterialComponentLit> MaterialComponents;

	EntityType LeaderType;
	EntityType FollowerType;

	bool SpawningEnabled = true;
	bool SpawnTimeLeft = true;

	float Speed = 6.0f;
	float Scale = 3.5f;
	float CollisionRadius = 1.01f;
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
