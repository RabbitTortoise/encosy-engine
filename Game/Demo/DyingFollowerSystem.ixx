module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Demo.Systems.DyingFollowerSystem;

import ECS.Entity;
import ECS.System;

import Components.TransformComponent;
import Demo.Components.DyingFollowerComponent;


import <map>;
import <span>;
import <vector>;
import <iostream>;



export class DyingFollowerSystem : public ThreadedSystem
{

	friend class SystemManager;

public:
	DyingFollowerSystem() {}
	~DyingFollowerSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		SystemQueueIndex = 1100;

		AddWantedComponentDataForWriting(&TransformComponents, &ThreadTransformComponentComponents);
		AddWantedComponentDataForWriting(&DyingFollowerComponents, &ThreadDyingFollowerComponents);

		ToBeDeleted = std::vector<std::vector<Entity>>(GetThreadCount());
	}
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override 
	{
		
	}
	void UpdatePerEntityThreaded(int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponentComponents);
		DyingFollowerComponent& dc = GetCurrentEntityComponent(thread, &ThreadDyingFollowerComponents);

		dc.TimeToLive -= deltaTime;
		if (dc.TimeToLive < 0)
		{
			ToBeDeleted[thread].emplace_back(entity);
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

	void PostUpdate(const double deltaTime) override 
	{
		for (auto& deleteVector : ToBeDeleted)
		{
			for (const auto& entity : deleteVector)
			{
				WorldEntityManager->DeleteEntity(entity);
			}
			deleteVector.clear();
		}
	}
	void Destroy() override {}

private:

	std::vector<std::vector<Entity>> ToBeDeleted;

	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<DyingFollowerComponent> DyingFollowerComponents;

	ThreadComponentStorage<TransformComponent> ThreadTransformComponentComponents;
	ThreadComponentStorage<DyingFollowerComponent> ThreadDyingFollowerComponents;
};
