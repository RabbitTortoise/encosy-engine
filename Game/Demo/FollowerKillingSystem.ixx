module;
#include <fmt/core.h>

export module Demo.Systems.FollowerKillingSystem;

import ECS.Entity;
import ECS.System;
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


export class FollowerKillingSystem : public ThreadedSystem
{

	struct EntityToChange
	{
		Entity entity;
		EntityType type;
	};

	friend class SystemManager;

public:

	FollowerKillingSystem(MeshLoader* meshLoader) { MainMeshLoader = meshLoader; }
	~FollowerKillingSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::PhysicsSystem;
		SystemQueueIndex = 1100;
		
		AddWantedComponentDataForReading(&FollowerComponents);
		AddWantedComponentDataForReading(&SphereColliderComponents);
		AddWantedComponentDataForReading(&TransformComponents);
		AddWantedComponentDataForWriting(&MovementComponents, &ThreadMovementComponents);
		AddWantedComponentDataForWriting(&MaterialComponents, &ThreadMaterialComponents);
		AddAlwaysFetchedComponentsForReading(&CollisionEvents);

		DisableAutomaticEntityLoop = true;

		EntitiesToChange = std::vector<std::vector<EntityToChange>>(GetThreadCount());

		IcoMeshID = MainMeshLoader->GetMeshID("DeadFollower.obj");
	}

	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override 
	{
		
		for(auto& changeVector : EntitiesToChange)
		{
			changeVector.clear();
		}

		ExecutePerEntityThreaded(std::bind_front(&FollowerKillingSystem::CustomUpdatePerEntityThreaded, this));

		for (auto& changeVector : EntitiesToChange)
		{
			for (auto& change : changeVector)
			{
				if (WorldEntityManager->ModifyEntityComponents<MovementComponent, DyingFollowerComponent>(change.entity, change.type))
				{
					auto mc = WorldEntityManager->GetWriteReadComponentFromEntity<MaterialComponentLit>(change.entity);
					auto dc = WorldEntityManager->GetWriteReadComponentFromEntity<DyingFollowerComponent>(change.entity);
					auto tc = WorldEntityManager->GetWriteReadComponentFromEntity<TransformComponent>(change.entity);
					mc->RenderMesh = IcoMeshID;
					dc->PushRange = tc->Scale.x;
					dc->TimeToLive = 5.0f;
				}
			}
		}
	}

	void UpdatePerEntityThreaded(int thread, const double deltaTime, Entity entity, EntityType entityType) override {}

	void CustomUpdatePerEntityThreaded(int thread, const double deltaTime, Entity entity, EntityType entityType) 
	{

		SphereColliderComponent ownCollider = GetCurrentEntityComponent(thread, &SphereColliderComponents);
		TransformComponent ownTransform = GetCurrentEntityComponent(thread, &TransformComponents);
		MaterialComponentLit& ownMaterial = GetCurrentEntityComponent(thread, &ThreadMaterialComponents);

		size_t vectorIndex = 0;
		float scaledRadius = ownTransform.Scale.x * ownCollider.Radius;
		for (const auto& span : CollisionEvents.Storage)
		{
			size_t spanIndex = 0;
			for (const auto& collisionEvent : span)
			{
				if(collisionEvent.A == entity)
				{
					if(collisionEvent.CollisionDepth > scaledRadius / 1.5f)
					{
						if (!WorldEntityManager->HasComponentType<DyingFollowerComponent>(collisionEvent.B))
						{
							auto otherMaterial = WorldEntityManager->GetReadOnlyComponentFromEntity<MaterialComponentLit>(collisionEvent.B);
							if (otherMaterial.Diffuse != ownMaterial.Diffuse)
							{
								EntitiesToChange[thread].push_back({ collisionEvent.A, entityType });
								EntitiesToChange[thread].push_back({ collisionEvent.B, entityType });
							}
						}
					}
				}
				spanIndex++;
			}
			vectorIndex++;
		}
	}

	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:
	std::vector<Entity> KilledThisFrame;

	std::vector<std::vector<EntityToChange>> EntitiesToChange;

	ReadOnlyComponentStorage<FollowerComponent> FollowerComponents;
	ReadOnlyComponentStorage<TransformComponent> TransformComponents;
	ReadOnlyComponentStorage<SphereColliderComponent> SphereColliderComponents;
	WriteReadComponentStorage<MovementComponent> MovementComponents;
	WriteReadComponentStorage<MaterialComponentLit> MaterialComponents;

	ThreadComponentStorage<MaterialComponentLit> ThreadMaterialComponents;
	ThreadComponentStorage<MovementComponent> ThreadMovementComponents;
	ReadOnlyComponentStorage<CollisionEventComponent> CollisionEvents;


	MeshLoader* MainMeshLoader;
	MeshID IcoMeshID;
};
