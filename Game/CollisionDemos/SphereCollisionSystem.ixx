module;
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>	
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo.Systems.SphereCollisionSystem;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import Components.TransformComponent;
import Demo.Components.SphereColliderComponent;
import Demo.Components.CollisionEventComponent;

import <map>;
import <span>;
import <vector>;
import <random>;
import <algorithm>;
import <functional>;


export class SphereCollisionSystem : public SystemThreaded
{
	friend class SystemManager;


	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = true,
	.AllowDestructiveEditsInThreads = true,
	.IgnoreThreadSaveFunctions = false,
	};

public:
	SphereCollisionSystem() {}
	~SphereCollisionSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::PhysicsSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		SetThreadedRunOptions(ThreadedRunOptions);

		AddComponentQueryForWriting(&TransformComponents, &ThreadTransformComponents);
		AddComponentQueryForReading(&SphereColliderComponents);
	}

	void PreUpdate(const int thread, const double deltaTime) override
	{
		ResetComponentStorage({ typeid(CollisionEventComponent), 0 });
	}

	void Update(const int thread, const double deltaTime) override {}

	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		const SphereColliderComponent sc = GetCurrentEntityComponent(thread, &SphereColliderComponents);
		const float scaledRadius = sc.Radius * tc.Scale.x;

		size_t vectorIndex = -1;
		for (const auto& span : TransformComponents.Storage)
		{
			vectorIndex++;
			size_t spanIndex = -1;
			const auto& entityTypeVec = FetchedEntitiesInfo[vectorIndex];
			for (const auto& tcOther : span)
			{
				spanIndex++;

				const float x = tc.Position.x - tcOther.Position.x;
				if (std::abs(x) > scaledRadius) 
				{ continue; }
				const float y = tc.Position.y - tcOther.Position.y;
				if (std::abs(y) > scaledRadius) 
				{ continue; }
				const float z = tc.Position.z - tcOther.Position.z;
				if (std::abs(z) > scaledRadius) 
				{ continue; }

				const Entity& otherEntity = entityTypeVec.EntityLocators[spanIndex].Entity;
				if (otherEntity == entity)
				{ continue; }

				const glm::vec3 dir = {x, y, z};
				const float radius2 = scaledRadius * scaledRadius;
				const float len2 = glm::length2(dir);
				if (len2 < radius2)
				{
					const float len = std::sqrt(len2);
					const float collisionDepth = scaledRadius - len;
					const glm::vec3 change = glm::normalize(dir) * collisionDepth;
					tc.Position += change;

					CreateNewComponentToStorage({ typeid(CollisionEventComponent), 0 }, CollisionEventComponent(entity, otherEntity, collisionDepth));
				}
			}		
		}
	}
	void PostUpdate(const int thread, const double deltaTime) override{}
	void Destroy() override {}

private:
	WriteReadComponentStorage<TransformComponent> TransformComponents;
	ReadOnlyComponentStorage<SphereColliderComponent> SphereColliderComponents;

	ThreadComponentStorage<TransformComponent> ThreadTransformComponents;
};
