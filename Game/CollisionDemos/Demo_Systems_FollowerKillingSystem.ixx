module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo_Systems_FollowerKillingSystem;

import EE_Encosy_Entity;
import EE_Encosy_SystemThreaded;

import EE_ECS_Components_TransformComponent;
import Demo_Components_DyingFollowerComponent;


import <map>;
import <span>;
import <vector>;
import <iostream>;



export class FollowerKillingSystem : public SystemThreaded
{

	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = true,
	.IgnoreThreadSaveFunctions = true,
	};

public:
	FollowerKillingSystem() {}
	~FollowerKillingSystem() {}


	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunAfterSpecificSystem = "DemoDyingFollowerSystem";
		SetThreadedRunOptions(ThreadedRunOptions);

		EnableDestructiveAccessToEntityStorage(GetEntityTypeInfo("DyingFollowerEntity").Type);
		AddComponentQueryForWriting(&DyingFollowerComponents, &ThreadDyingFollowerComponents);
	}
	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override {}
	void UpdatePerEntity(int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		DyingFollowerComponent& dc = GetCurrentEntityComponent(thread, &ThreadDyingFollowerComponents);

		dc.TimeToLive -= deltaTime;
		if (dc.TimeToLive < 0)
		{
			DeleteEntity(entity, entityType);
		}
	}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:

	WriteReadComponentStorage<DyingFollowerComponent> DyingFollowerComponents;
	ThreadComponentStorage<DyingFollowerComponent> ThreadDyingFollowerComponents;
};
