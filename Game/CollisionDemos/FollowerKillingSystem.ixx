module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo.Systems.FollowerKillingSystem;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;

import Components.TransformComponent;
import Demo.Components.DyingFollowerComponent;


import <map>;
import <span>;
import <vector>;
import <iostream>;



export class FollowerKillingSystem : public SystemThreaded
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
	FollowerKillingSystem() {}
	~FollowerKillingSystem() {}

protected:
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
			DeleteEntity(entity);
		}
	}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:

	WriteReadComponentStorage<DyingFollowerComponent> DyingFollowerComponents;
	ThreadComponentStorage<DyingFollowerComponent> ThreadDyingFollowerComponents;
};
