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
	.IgnoreThreadSaveFunctions = true,
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
		SystemThreadInfo threadInfo = GetThreadRuntimeInfo(thread);

		TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		const SphereColliderComponent sc = GetCurrentEntityComponent(thread, &SphereColliderComponents);
		const float scaledRadius = sc.Radius * tc.Scale.x;

		for (size_t outer = threadInfo.outerIndex; outer < TransformComponents.Storage.size(); outer++)
		{
			const auto& entityTypeVec = FetchedEntitiesInfo[outer];
			for (size_t inner = threadInfo.innerIndexRead + 1; inner < TransformComponents.Storage[outer].size(); inner++)
			{
				TransformComponent& tcOther = TransformComponents.Storage[outer][inner];
				const auto& colliderOther = SphereColliderComponents.Storage[outer][inner];
				const float scaledRadiusOther = colliderOther.Radius * tcOther.Scale.x;
				const float collisionDistance = scaledRadius + scaledRadiusOther;

				const float x = tc.Position.x - tcOther.Position.x;
				if (std::abs(x) > collisionDistance) {continue;}

				const float y = tc.Position.y - tcOther.Position.y;
				if (std::abs(y) > collisionDistance){continue;}

				const float z = tc.Position.z - tcOther.Position.z;
				if (std::abs(z) > collisionDistance){continue;}

				const glm::vec3 dir = { x, y, z };
				const float radius2 = collisionDistance * collisionDistance;
				const float len2 = glm::length2(dir);
				if (len2 < radius2)
				{
					const float len = std::sqrt(len2);
					const float collisionDepth = collisionDistance - len;
					glm::vec3 change = glm::normalize(dir) * collisionDepth / 2.0f;
					const Entity otherEntity = entityTypeVec.EntityLocators[outer].Entity;
					if (!sc.Unmovable || !colliderOther.Unmovable)
					{
						change = change * 2.0f;
					}
					if (!sc.Unmovable)
					{
						TransformComponent& tcRef = TransformComponents.Storage[threadInfo.outerIndex][threadInfo.innerIndexRead];
						tcRef.Position += change;
						//SetEntityComponent(entity, entityType, tc);  // This function is not thread safe yet
					}
					if (!colliderOther.Unmovable)
					{
						tcOther.Position -= change;
						//SetEntityComponent(otherEntity, entityTypeVec.Type, tcOtherNew); // This function is not thread safe yet
					}
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
