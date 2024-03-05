module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo.Systems.SpawningSystem;

import ECS.Entity;
import ECS.System;
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


export class SpawningSystem : public System
{
	friend class SystemManager;

public:
	SpawningSystem(TextureLoader* textureLoader, MeshLoader* meshLoader) : MainTextureLoader(textureLoader), MainMeshLoader(meshLoader) {}
	~SpawningSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		SystemQueueIndex = 2000;

		DisableAutomaticEntityLoop = true;

		FollowerType = WorldEntityManager->GetEntityTypeInfo("FollowerEntity").Type;
		LeaderType = WorldEntityManager->GetEntityTypeInfo("LeaderEntity").Type;

		AddSystemDataForWriting(&SpawningSystemDataComponent);
		AddAlwaysFetchedEntitiesForReading(LeaderType, &LeaderComponents);
		AddAlwaysFetchedEntitiesForReading(LeaderType, &LeaderTransformComponents);

		std::random_device rd;
		std::mt19937 gen(rd());
		Gen = gen;


	}
	void PreUpdate(const double deltaTime) override 
	{
		LeadersPerID.clear();
		for (size_t i = 0; i < LeaderComponents.Storage.size(); i++)
		{
			auto it = LeadersPerID.find(LeaderComponents.Storage[i].LeaderID);
			if (it == LeadersPerID.end())
			{
				LeadersPerID.insert(std::pair(LeaderComponents.Storage[i].LeaderID, std::vector<size_t>()));
				it = LeadersPerID.find(LeaderComponents.Storage[i].LeaderID);
			}
			it->second.push_back(i);
		}
	}

	void Update(const double deltaTime) override
	{
		auto& systemData = GetSystemData(&SpawningSystemDataComponent);


		LastFrametimes[CurrentFrameTimeIndex] = deltaTime;
		CurrentFrameTimeIndex++;
		if (CurrentFrameTimeIndex == LastFrametimes.size()) { CurrentFrameTimeIndex = 0; }
		double averageFPS = 1.0 / (std::accumulate(LastFrametimes.begin(), LastFrametimes.end(), LastFrametimes[0], [](const double a, const double b)
			{
				return a + b;
			}) / LastFrametimes.size());

		LastSpawnTime += deltaTime;

		while (LastSpawnTime - SpawnInterval > 0)
		{
			LastSpawnTime -= SpawnInterval;

			if (averageFPS < 110.0f) { SpawningEnabled = false; }
			if (!SpawningEnabled)
			{
				if (averageFPS > 118.0f) { SpawningEnabled = true; }
				return;
			}

			std::uniform_real_distribution<float> distr(0.0f, 1.0f);

			int leaderIndex = std::round(distr(Gen) * (systemData.CurrentLeaders.size() - 1));
			int leader = systemData.CurrentLeaderIndexes[leaderIndex];
			MaterialComponentLit leaderMaterial = systemData.LeaderMaterial[leaderIndex];

			std::uniform_real_distribution<float> distrX(systemData.PlayRegionMin.x, systemData.PlayRegionMax.x);
			std::uniform_real_distribution<float> distrY(systemData.PlayRegionMin.y, systemData.PlayRegionMax.y);
			std::uniform_real_distribution<float> distrZ(systemData.PlayRegionMin.z, systemData.PlayRegionMax.z);

			std::uniform_real_distribution<float> randVariation(-5.0f, 5.0f);

			TransformComponent tc = { {distrX(Gen),distrY(Gen),distrZ(Gen) },  {Scale,Scale,Scale},  glm::quat() };
			
			float closestLen = std::numeric_limits<float>::max();
			size_t closestIndex = 0;
			const auto& leadersIndexes = LeadersPerID.find(leader)->second;
			for (auto leaderIndex : leadersIndexes)
			{
				TransformComponent ltc = LeaderTransformComponents.Storage[leaderIndex];
				glm::vec3 dirToTarget = ltc.Position - tc.Position;
				float len = glm::length(dirToTarget);
				if (len < closestLen)
				{
					closestLen = len;
					closestIndex = leader;
				}
			}
			TransformComponent ltc = LeaderTransformComponents.Storage[closestIndex];
			tc.Position = ltc.Position;
			tc.Position.x += randVariation(Gen);
			tc.Position.y += randVariation(Gen);
			tc.Position.z += randVariation(Gen);
			
			
			MaterialComponentLit mat = { leaderMaterial.Diffuse,leaderMaterial.Normal, MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere), leaderMaterial.TextureRepeat };
			ModelMatrixComponent matrix = {};
			FollowerComponent fc = { leader };
			MovementComponent mov = { {}, {Speed} };
			SphereColliderComponent sphere = { CollisionRadius };
			WorldEntityManager->CreateEntityWithData(FollowerType, tc, mat, matrix, fc, mov, sphere);
			SpawnedEntities++;
		}
	}
	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override {}

	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:

	WriteReadSystemDataStorage<SpawningSystemData> SpawningSystemDataComponent;
	ReadOnlyAlwaysFetchedStorage<LeaderComponent> LeaderComponents;
	ReadOnlyAlwaysFetchedStorage<TransformComponent> LeaderTransformComponents; 

	EntityType LeaderType;
	EntityType FollowerType;
	std::map<int, std::vector<size_t>> LeadersPerID;

	bool SpawningEnabled = true;

	float Speed = 6.0f;
	float Scale = 4.0f;
	float CollisionRadius = 1.01f;
	float SpawnInterval = 0.01f;
	float LastSpawnTime = 0.0f;
	size_t SpawnedEntities = 0;

	std::mt19937 Gen;

	std::vector<TextureID> textureIds;
	std::vector<MeshID> meshIDs;

	TextureLoader* MainTextureLoader;
	MeshLoader* MainMeshLoader;

	std::vector<double> LastFrametimes = std::vector<double>(120, 0);
	int CurrentFrameTimeIndex = 0;
};
