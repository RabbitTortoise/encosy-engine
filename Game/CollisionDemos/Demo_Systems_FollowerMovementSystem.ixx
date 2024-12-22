module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo_Systems_FollowerMovementSystem;

import EE_Encosy_Entity;
import EE_Encosy_SystemThreaded;
import EE_ECS_Components_TransformComponent;
import Demo_Components_MovementComponent;
import Demo_Components_FollowerComponent;
import Demo_Components_LeaderComponent;

import <map>;
import <span>;
import <vector>;
import <random>;
import <algorithm>;
import <numeric>;


export class FollowerMovementSystem : public SystemThreaded
{

public:
	FollowerMovementSystem() {}
	~FollowerMovementSystem() {}

	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = false,
	.IgnoreThreadSaveFunctions = false,
	};

	void Init() override
	{
		Type = SystemType::PhysicsSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunBeforeSpecificSystem = "DemoSphereCollisionSystem";
		SetThreadedRunOptions(ThreadedRunOptions);

		LeaderType = GetEntityTypeInfo("LeaderEntity").Type;

		AddComponentQueryForWriting(&TransformComponents, &ThreadTransformComponents);
		AddComponentQueryForWriting(&MovementComponents, &ThreadMovementComponents);
		AddComponentQueryForReading(&FollowerComponents);

		AddEntitiesForReading(LeaderType, &LeaderComponents);
		AddEntitiesForReading(LeaderType, &LeaderTransformComponents);
	}

	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override
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

	void UpdatePerEntity(int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		EE_TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		MovementComponent& mc = GetCurrentEntityComponent(thread, &ThreadMovementComponents);
		FollowerComponent fc = GetCurrentEntityComponent(thread, &FollowerComponents);

		float closestLen = std::numeric_limits<float>::max();
		size_t closestIndex = 0;
		const auto& leaders = LeadersPerID.find(fc.LeaderToFollow)->second;
		for (auto leader : leaders)
		{
			EE_TransformComponent ltc = LeaderTransformComponents.Storage[leader];
			glm::vec3 dirToTarget = ltc.Position - tc.Position;
			float len = glm::length(dirToTarget);
			if (len < closestLen)
			{
				closestLen = len;
				closestIndex = leader;
			}
		}
		EE_TransformComponent ltc = LeaderTransformComponents.Storage[closestIndex];
		glm::vec3 dirToTarget = glm::normalize(ltc.Position - tc.Position);
		mc.Direction += dirToTarget;

		mc.Direction = glm::normalize(mc.Direction);
		tc.Position += mc.Direction * static_cast<float>(mc.Speed * deltaTime);
	}
	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:
	float DistanceToOthers = 5.0f;

	EntityType LeaderType;
	std::map<int, std::vector<size_t>> LeadersPerID;

	WriteReadComponentStorage<EE_TransformComponent> TransformComponents;
	WriteReadComponentStorage<MovementComponent> MovementComponents;
	ReadOnlyComponentStorage<FollowerComponent> FollowerComponents;

	ReadOnlyAlwaysFetchedStorage<LeaderComponent> LeaderComponents;
	ReadOnlyAlwaysFetchedStorage<EE_TransformComponent> LeaderTransformComponents;


	ThreadComponentStorage<EE_TransformComponent> ThreadTransformComponents;
	ThreadComponentStorage<MovementComponent> ThreadMovementComponents;
};
