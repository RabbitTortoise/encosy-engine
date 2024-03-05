module;
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>	
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo.Systems.SphereCollisionSystem;

import ECS.Entity;
import ECS.System;
import Components.TransformComponent;
import Demo.Components.SphereColliderComponent;
import Demo.Components.CollisionEventComponent;

import <map>;
import <span>;
import <vector>;
import <random>;
import <algorithm>;
import <functional>;


export class SphereCollisionSystem : public ThreadedSystem
{
	friend class SystemManager;

public:
	SphereCollisionSystem() {}
	~SphereCollisionSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::PhysicsSystem;
		SystemQueueIndex = 1100;

		AddWantedComponentDataForWriting(&TransformComponents, &ThreadTransformComponents);
		AddWantedComponentDataForReading(&SphereColliderComponents);

		CollsionEventType = WorldEntityManager->GetEntityTypeInfo("CollisionEvent").Type;

		CollisionEvents = std::vector< std::vector<CollisionEventComponent>>(GetThreadCount(), std::vector<CollisionEventComponent>());

		DisableAutomaticEntityLoop = true;

	}
	void PreUpdate(const double deltaTime) override
	{
		WorldComponentManager->ResetStorage({ typeid(CollisionEventComponent), 0 });
	}
	void Update(const double deltaTime) override 
	{
		ExecutePerEntityThreaded(std::bind_front(&SphereCollisionSystem::CustomUpdatePerEntityThreaded, this));

		for (auto& threadVector : CollisionEvents)
		{
			for (const auto& collision : threadVector)
			{
				WorldComponentManager->CreateNewComponentToStorage({ typeid(CollisionEventComponent), 0 }, collision);
			}
			threadVector.clear();
		}
	}

	void UpdatePerEntityThreaded(int thread, const double deltaTime, Entity entity, EntityType entityType) override {}

	void CustomUpdatePerEntityThreaded(int thread, const double deltaTime, Entity entity, EntityType entityType)
	{
		TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		const SphereColliderComponent sc = GetCurrentEntityComponent(thread, &SphereColliderComponents);
		const float scaledRadius = sc.Radius * tc.Scale.x;
		const float radius2 = scaledRadius * scaledRadius;

		size_t vectorIndex = -1;
		for (const auto& span : TransformComponents.Storage)
		{
			vectorIndex++;
			size_t spanIndex = -1;
			for (const auto& tcOther : span)
			{
				spanIndex++;
				const Entity otherEntity = FetchedEntitiesInfo[vectorIndex].EntityLocators[spanIndex].Entity;
				if (otherEntity == entity) { continue; }

				float x = tc.Position.x - tcOther.Position.x;
				if (std::abs(x) > scaledRadius) { continue; }
				float y = tc.Position.y - tcOther.Position.y;
				if (std::abs(y) > scaledRadius) { continue; }
				float z = tc.Position.z - tcOther.Position.z;
				if (std::abs(z) > scaledRadius) { continue; }

				glm::vec3 dir = {x, y, z};
				const float len2 = glm::length2(dir);
				if (len2 < radius2)
				{
					const float len = std::sqrt(len2);
					const float collisionDepth = scaledRadius - len;
					const glm::vec3 change = glm::normalize(dir) * collisionDepth;
					tc.Position += change;

					CollisionEvents[thread].emplace_back(CollisionEventComponent(entity, otherEntity, collisionDepth));
				}
			}		
		}
	}
	void PostUpdate(const double deltaTime) override
	{
		
	}
	void Destroy() override {}

private:

	EntityType CollsionEventType;
	std::vector<std::vector<CollisionEventComponent>> CollisionEvents;

	WriteReadComponentStorage<TransformComponent> TransformComponents;
	ReadOnlyComponentStorage<SphereColliderComponent> SphereColliderComponents;

	ThreadComponentStorage<TransformComponent> ThreadTransformComponents;
};
