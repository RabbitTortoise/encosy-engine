module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo.Systems.LeaderMovementSystem;

import ECS.Entity;
import ECS.System;
import Components.TransformComponent;
import Demo.Components.MovementComponent;
import Demo.Components.LeaderComponent;
import Demo.SystemData.SpawningSystem;

import <map>;
import <span>;
import <vector>;
import <random>;


export class LeaderMovementSystem : public System
{
	friend class SystemManager;

public:
	LeaderMovementSystem() {}
	~LeaderMovementSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::PhysicsSystem;
		SystemQueueIndex = 1100;


		AddWantedComponentDataForWriting(&TransformComponents);
		AddWantedComponentDataForWriting(&MovementComponents);
		AddWantedComponentDataForWriting(&LeaderComponents);

		AddSystemDataForReading(&SpawningSystemDataComponent);

		std::random_device rd;
		std::mt19937 gen(rd());
		Gen = gen;

	}
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override
	{
	}
	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override
	{
		TransformComponent& tc = GetCurrentEntityComponent(&TransformComponents);
		MovementComponent& mc = GetCurrentEntityComponent(&MovementComponents);
		LeaderComponent& lc = GetCurrentEntityComponent(&LeaderComponents);
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
	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:

	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<MovementComponent> MovementComponents;
	WriteReadComponentStorage<LeaderComponent> LeaderComponents;

	ReadOnlySystemDataStorage<SpawningSystemData> SpawningSystemDataComponent;

	std::mt19937 Gen;

};