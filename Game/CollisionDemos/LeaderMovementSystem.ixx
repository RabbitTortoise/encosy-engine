module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo.Systems.LeaderMovementSystem;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import Components.TransformComponent;
import Demo.Components.MovementComponent;
import Demo.Components.LeaderComponent;
import Demo.SystemData.SpawningSystem;

import <map>;
import <span>;
import <vector>;
import <random>;


export class LeaderMovementSystem : public SystemThreaded
{
	friend class SystemManager;

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

protected:
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
		TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
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
	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<MovementComponent> MovementComponents;

	ThreadComponentStorage<LeaderComponent> ThreadLeaderComponents;
	ThreadComponentStorage<TransformComponent> ThreadTransformComponents;
	ThreadComponentStorage<MovementComponent> ThreadMovementComponents;

	ReadOnlySystemDataStorage<SpawningSystemData> SpawningSystemDataComponent;

	std::mt19937 Gen;

};