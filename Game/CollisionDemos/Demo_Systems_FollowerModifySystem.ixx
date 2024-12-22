module;
#include <fmt/core.h>
#include <glm/vec3.hpp>

export module Demo_Systems_FollowerModifySystem;

import EE_Encosy_Entity;
import EE_Encosy_SystemThreaded;
import EE_RenderCore_MeshLoader;
import EE_RenderCore_VulkanTypes;

import EE_ECS_Components_TransformComponent;
import EE_ECS_Components_MaterialComponent;
import Demo_Components_FollowerComponent;
import Demo_Components_MovementComponent;
import Demo_Components_SphereColliderComponent;
import Demo_Components_CollisionEventComponent;
import Demo_Components_DyingFollowerComponent;


import <map>;
import <span>;
import <vector>;
import <iostream>;
import <functional>;


export class FollowerModifySystem : public SystemThreaded
{

	struct EntityToChange
	{
		Entity entity;
		EntityType type;
	};


	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = true,
	.IgnoreThreadSaveFunctions = true,
	};

public:

	FollowerModifySystem(MeshLoader* meshLoader)
	{
		MainMeshLoader = meshLoader;
	}
	~FollowerModifySystem() {}


	void Init() override
	{
		Type = SystemType::PhysicsSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		SetThreadedRunOptions(ThreadedRunOptions);

		FollowerEntityType = GetEntityTypeInfo("FollowerEntity").Type;
		DyingFollowerEntityType = GetEntityTypeInfo("DyingFollowerEntity").Type;

		EnableDestructiveAccessToEntityStorage(FollowerEntityType);
		EnableDestructiveAccessToEntityStorage(DyingFollowerEntityType);

		AddRequiredComponentQuery<FollowerComponent>();
		AddRequiredComponentQuery<MovementComponent>();
		
		AddComponentQueryForWriting(&SphereColliderComponents, &ThreadSphereColliderComponents);
		AddComponentQueryForWriting(&TransformComponents, &ThreadTransformComponents);
		AddComponentQueryForWriting(&RaytraceMaterialComponents, &ThreadRaytraceMaterialComponents);
		AddComponentsForReading(&CollisionEvents);

		IcoMeshID = MainMeshLoader->GetMeshID("DeadFollower.obj");
	}

	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override {}

	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		SphereColliderComponent& ownCollider = GetCurrentEntityComponent(thread, &ThreadSphereColliderComponents);
		EE_TransformComponent& ownTransform = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		EE_MaterialComponent& ownMaterial = GetCurrentEntityComponent(thread, &ThreadRaytraceMaterialComponents);

		size_t vectorIndex = 0;
		float scaledRadius = ownTransform.Scale.x * ownCollider.Radius;
		for (const auto& span : CollisionEvents.Storage)
		{
			size_t spanIndex = 0;
			for (const auto& collisionEvent : span)
			{
				if (collisionEvent.A != entity) { continue; }

				if (collisionEvent.CollisionDepth < scaledRadius / 1.5f) { continue; }
				if (HasComponentType<DyingFollowerComponent>(collisionEvent.B)) { continue; }

				auto otherMaterial = GetReadOnlyComponentFromEntity<EE_MaterialComponent>(collisionEvent.B);
				if (otherMaterial.TextureSet == ownMaterial.TextureSet) { continue; }

				DyingFollowerComponent newDc;
				newDc.PushRange = ownTransform.Scale.x;
				newDc.TimeToLive = 5.0f;

				auto newRmc = ownMaterial;
				newRmc.RenderMesh = IcoMeshID;
				newRmc.Color = glm::vec3(1.0f, 0.4f, 0.4f);
				ModifyEntityComponents<MovementComponent>(entity, FollowerEntityType, DyingFollowerEntityType, newDc, newRmc);

			}
		}
	}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:

	WriteReadComponentStorage<SphereColliderComponent> SphereColliderComponents;
	WriteReadComponentStorage<EE_TransformComponent> TransformComponents;
	WriteReadComponentStorage<EE_MaterialComponent> RaytraceMaterialComponents;

	ThreadComponentStorage<SphereColliderComponent> ThreadSphereColliderComponents;
	ThreadComponentStorage<EE_TransformComponent> ThreadTransformComponents;
	ThreadComponentStorage<EE_MaterialComponent> ThreadRaytraceMaterialComponents;

	ReadOnlyComponentStorage<CollisionEventComponent> CollisionEvents;

	MeshLoader* MainMeshLoader;
	MeshID IcoMeshID;

	EntityType FollowerEntityType = -1;
	EntityType DyingFollowerEntityType = -1;
};
