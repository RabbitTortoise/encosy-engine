module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo_Systems_LeaderMovementSystem;

import EE_Encosy_Entity;
import EE_Encosy_SystemThreaded;
import EE_ECS_Components_TransformComponent;
import Demo_Components_MovementComponent;
import Demo_Components_LeaderComponent;
import Demo_SystemData_SpawningSystem;

import <map>;
import <span>;
import <vector>;
import <random>;


export class LeaderMovementSystem : public SystemThreaded
{
	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = false,
	.IgnoreThreadSaveFunctions = false,
	};

public:
	LeaderMovementSystem() {}
	~LeaderMovementSystem() {}

	void Init() override
	{
		Type = SystemType::PhysicsSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunBeforeSpecificSystem = "DemoSphereCollisionSystem";
		SetThreadedRunOptions(ThreadedRunOptions);


		AddComponentQueryForWriting(&LeaderComponents, &ThreadLeaderComponents);
		AddComponentQueryForWriting(&TransformComponents, &ThreadTransformComponents);
		AddComponentQueryForWriting(&MovementComponents, &ThreadMovementComponents);

		AddSystemDataForReading(&SpawningSystemDataComponent);

		std::random_device rd;
		std::mt19937 gen(rd());
		Gen = gen;

	}
	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override
	{
	}
	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		EE_TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		MovementComponent& mc = GetCurrentEntityComponent(thread, &ThreadMovementComponents);
		LeaderComponent& lc = GetCurrentEntityComponent(thread, &ThreadLeaderComponents);
		auto systemData = GetSystemData(&SpawningSystemDataComponent);

		if (glm::length(lc.TargetPoint - tc.Position) < 1.0f)
		{

			std::uniform_real_distribution<float> distrX(systemData.PlayRegionMin.x, systemData.PlayRegionMax.x);
			std::uniform_real_distribution<float> distrY(systemData.PlayRegionMin.y, systemData.PlayRegionMax.y);
			std::uniform_real_distribution<float> distrZ(systemData.PlayRegionMin.z, systemData.PlayRegionMax.z);
			glm::vec3 newTarget = glm::vec3(distrX(Gen), distrY(Gen), distrZ(Gen));
			glm::vec3 newTarget2 = glm::vec3(distrX(Gen), distrY(Gen), distrZ(Gen));

			if (std::abs(newTarget.x) > std::abs(newTarget2.x))
			{
				lc.TargetPoint = newTarget;
			}
			else
			{
				lc.TargetPoint = newTarget2;
			}
		}

		mc.Direction = glm::normalize(lc.TargetPoint - tc.Position);
		tc.Position += mc.Direction * static_cast<float>(mc.Speed * deltaTime);
	}
	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:

	WriteReadComponentStorage<LeaderComponent> LeaderComponents;
	WriteReadComponentStorage<EE_TransformComponent> TransformComponents;
	WriteReadComponentStorage<MovementComponent> MovementComponents;

	ThreadComponentStorage<LeaderComponent> ThreadLeaderComponents;
	ThreadComponentStorage<EE_TransformComponent> ThreadTransformComponents;
	ThreadComponentStorage<MovementComponent> ThreadMovementComponents;

	ReadOnlySystemDataStorage<SpawningSystemData> SpawningSystemDataComponent;

	std::mt19937 Gen;

};