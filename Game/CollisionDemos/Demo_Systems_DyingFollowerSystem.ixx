module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo_Systems_DyingFollowerSystem;

import EE_Encosy_Entity;
import EE_Encosy_SystemThreaded;

import EE_ECS_Components_TransformComponent;
import EE_ECS_Components_MaterialComponent;
import Demo_Components_DyingFollowerComponent;


import <map>;
import <span>;
import <vector>;



export class DyingFollowerSystem : public SystemThreaded
{

	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = false,
	.IgnoreThreadSaveFunctions = false,
	};

public:
	DyingFollowerSystem() {}
	~DyingFollowerSystem() {}


	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		SetThreadedRunOptions(ThreadedRunOptions);

		AddComponentQueryForWriting(&TransformComponents, &ThreadTransformComponentComponents);
		AddComponentQueryForWriting(&DyingFollowerComponents, &ThreadDyingFollowerComponents);
	}

	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override {	}
	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		EE_TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponentComponents);
		DyingFollowerComponent& dc = GetCurrentEntityComponent(thread, &ThreadDyingFollowerComponents);

		dc.TimeToLive -= deltaTime;
		if (dc.TimeToLive < 0)
		{
			return;
		}

		float change = deltaTime * 0.5f;
		if (dc.TimeToLive < 2)
		{
			change *= (-2.5f);
		}

		dc.PushRange += change;
		tc.Scale.x = dc.PushRange;
		tc.Scale.y = dc.PushRange;
		tc.Scale.z = dc.PushRange;
		if (tc.Scale.x < 0.1f)
		{
			tc.Scale.x = 0.1f;
		}
		if (tc.Scale.y < 0.1f)
		{
			tc.Scale.y = 0.1f;
		}
		if (tc.Scale.z < 0.1f)
		{
			tc.Scale.z = 0.1f;
		}
	}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:

	WriteReadComponentStorage<EE_TransformComponent> TransformComponents;
	WriteReadComponentStorage<DyingFollowerComponent> DyingFollowerComponents;

	ThreadComponentStorage<EE_TransformComponent> ThreadTransformComponentComponents;
	ThreadComponentStorage<DyingFollowerComponent> ThreadDyingFollowerComponents;
};
