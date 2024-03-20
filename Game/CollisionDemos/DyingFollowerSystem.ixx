module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo.Systems.DyingFollowerSystem;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;

import Components.TransformComponent;
import Demo.Components.DyingFollowerComponent;


import <map>;
import <span>;
import <vector>;
import <iostream>;



export class DyingFollowerSystem : public SystemThreaded
{

	friend class SystemManager;

public:
	DyingFollowerSystem() {}
	~DyingFollowerSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;

		AddComponentQueryForWriting(&TransformComponents, &ThreadTransformComponentComponents);
		AddComponentQueryForWriting(&DyingFollowerComponents, &ThreadDyingFollowerComponents);
	}
	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override {	}
	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponentComponents);
		DyingFollowerComponent& dc = GetCurrentEntityComponent(thread, &ThreadDyingFollowerComponents);

		dc.TimeToLive -= deltaTime;
		if (dc.TimeToLive < 0)
		{
			return;
		}

		float change = deltaTime * 1.0f;
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

	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<DyingFollowerComponent> DyingFollowerComponents;

	ThreadComponentStorage<TransformComponent> ThreadTransformComponentComponents;
	ThreadComponentStorage<DyingFollowerComponent> ThreadDyingFollowerComponents;
};
