module;
#include <fmt/core.h>
#include <glm/vec3.hpp>

export module Demo.Systems.FollowerModifySystem;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import RenderCore.MeshLoader;
import RenderCore.VulkanTypes;

import Components.TransformComponent;
import Components.MaterialComponent;
import Demo.Components.FollowerComponent;
import Demo.Components.MovementComponent;
import Demo.Components.SphereColliderComponent;
import Demo.Components.CollisionEventComponent;
import Demo.Components.DyingFollowerComponent;


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

	FollowerModifySystem(MeshLoader* meshLoader)
	{
		MainMeshLoader = meshLoader;
	}
	~FollowerModifySystem() {}

protected:
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
		TransformComponent& ownTransform = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		MaterialComponentRaytracing& ownMaterial = GetCurrentEntityComponent(thread, &ThreadRaytraceMaterialComponents);

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

				auto otherMaterial = GetReadOnlyComponentFromEntity<MaterialComponentRaytracing>(collisionEvent.B);
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
	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<MaterialComponentRaytracing> RaytraceMaterialComponents;

	ThreadComponentStorage<SphereColliderComponent> ThreadSphereColliderComponents;
	ThreadComponentStorage<TransformComponent> ThreadTransformComponents;
	ThreadComponentStorage<MaterialComponentRaytracing> ThreadRaytraceMaterialComponents;

	ReadOnlyComponentStorage<CollisionEventComponent> CollisionEvents;

	MeshLoader* MainMeshLoader;
	MeshID IcoMeshID;

	EntityType FollowerEntityType = -1;
	EntityType DyingFollowerEntityType = -1;
};
