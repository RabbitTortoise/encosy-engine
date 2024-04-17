module;

export module Thesis.Systems.ThesisEntityModifySystemMt8;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import RenderCore.MeshLoader;
import RenderCore.VulkanTypes;

import Components.TransformComponent;
import Components.MaterialComponent;
import Thesis.Components;

import <vector>;
import <array>;


export class ThesisEntityModifySystemMt8 : public SystemThreaded
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

	ThesisEntityModifySystemMt8(bool allowConcurrent)
	{
		ThreadedRunOptions.PreferRunAlone = !allowConcurrent;
	}
	~ThesisEntityModifySystemMt8() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		SetThreadedRunOptions(ThreadedRunOptions);

		RunWithSpecificSystem = "ThesisEntityModifySystemMt1";

		AddRequiredComponentQuery<EntityTypeComponent8>();
		AddForbiddenComponentQuery<DestroyEntityComponent>();

		SystemEntityType = GetEntityTypeInfo("EntityType8").Type;
		SystemEntityToDestroyType = GetEntityTypeInfo("EntityToDestroy8").Type;

		EnableDestructiveAccessToEntityStorage(SystemEntityType);
		EnableDestructiveAccessToEntityStorage(SystemEntityToDestroyType);
	}

	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override
	{
		DestroyEntityComponent dec;
		for (const auto& fetchedEntityInfo : FetchedEntitiesInfo)
		{
			for (const auto& locator : fetchedEntityInfo.EntityLocators)
			{
				ModifyEntityComponents<RotateComponent>(locator.Entity, SystemEntityType, SystemEntityToDestroyType, dec);
			}
		}
	}

	void UpdatePerEntity(const int thread, const double deltaTime, const Entity entity, const EntityType entityType) override {}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}


private:
	EntityType SystemEntityType = -1;
	EntityType SystemEntityToDestroyType = -1;
};
