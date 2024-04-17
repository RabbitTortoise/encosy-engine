module;
#include <glm/glm.hpp>	

export module Thesis.Systems.ThesisEntityDestroySystemMt4;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import Thesis.Components;


export class ThesisEntityDestroySystemMt4 : public SystemThreaded
{
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
	ThesisEntityDestroySystemMt4(bool allowConcurrent)
	{
		ThreadedRunOptions.PreferRunAlone = !allowConcurrent;
	}
	~ThesisEntityDestroySystemMt4() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunAfterSpecificSystem = "ThesisEntitySpawningSystemMt10";
		RunWithSpecificSystem = "ThesisEntityDestroySystemMt1";
		RunBeforeSpecificSystem = "ThesisRotationSystemMt";
		SetThreadedRunOptions(ThreadedRunOptions);

		AddRequiredComponentQuery<EntityTypeComponent4>();
		AddRequiredComponentQuery<DestroyEntityComponent>();

		SystemEntityType = GetEntityTypeInfo("EntityToDestroy4").Type;
		EnableDestructiveAccessToEntityStorage(SystemEntityType);

	}
	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override
	{
		DestroyEntityComponent dec;
		for (const auto& fetchedEntityInfo : FetchedEntitiesInfo)
		{
			for (const auto& locator : fetchedEntityInfo.EntityLocators)
			{
				bool success = DeleteEntity(locator.Entity, SystemEntityType);
			}
		}
	}
	void UpdatePerEntity(const int thread, const double deltaTime, const Entity entity, const EntityType entityType) override {}
	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:
	EntityType SystemEntityType = -1;
};