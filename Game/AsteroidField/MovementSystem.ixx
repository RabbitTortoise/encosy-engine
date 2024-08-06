module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module AsteroidField.Systems.MovementSystem;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import Components.TransformComponent;
import AsteroidField.Components.MovementComponent;
import EncosyEngine.MatrixCalculations;
import SystemData.InputSystem;

import <map>;
import <span>;
import <vector>;
import <random>;
import <algorithm>;
import <numeric>;


export class AsteroidFieldMovementSystem : public SystemThreaded
{
	friend class SystemManager;

public:
	AsteroidFieldMovementSystem() {}
	~AsteroidFieldMovementSystem() {}

	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = false,
	.IgnoreThreadSaveFunctions = false,
	};

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		SetThreadedRunOptions(ThreadedRunOptions);

		AddSystemDataForReading(&InputSystemDataStorage);

		AddComponentQueryForWriting(&TransformComponents, &ThreadTransformComponents);
		AddComponentQueryForWriting(&MovementComponents, &ThreadMovementComponents);
	}

	/*
export struct AsteroidFieldMovementComponent
{
	float goalAngleX;
	float goalAngleY;
	float radius;
	glm::vec3 goalPosition;
	glm::vec2 angleDirection;
	float speed;
};
};
	*/

	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override 
	{
		InputData = InputSystemDataStorage.Storage[0];
		if (InputData.Spacebar && !PauseToggled)
		{
			Paused = !Paused;
			PauseToggled = !PauseToggled;
		}
		if (!InputData.Spacebar && PauseToggled)
		{
			PauseToggled = !PauseToggled;
		}
	}

	void UpdatePerEntity(int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{

		if (Paused)
		{
			return;
		}

		TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		AsteroidFieldMovementComponent& mc = GetCurrentEntityComponent(thread, &ThreadMovementComponents);

		// Rotate towards center
		glm::vec3 lookAt = -tc.Position;
		tc.Orientation = MatrixCalculations::LookAtQuaternion(lookAt, glm::vec3(0, 1, 0), true);

		glm::vec3 dir = mc.goalPosition - tc.Position;
		float len = glm::length(dir);
		float positionAdd = mc.speed * static_cast<float>(deltaTime);

		glm::vec3 normalized = glm::normalize(dir);
		if (len > positionAdd)
		{
			tc.Position += normalized * positionAdd;
			return;
		}

		if (len != 0)
		{
			tc.Position += normalized * len;
		}

		mc.prevAngle = mc.goalAngle;
		mc.goalAngle += mc.angleDirection;

		mc.goalPosition.x = mc.radius * sin(mc.goalAngle.x) * cos(mc.goalAngle.y);
		mc.goalPosition.y = mc.radius * sin(mc.goalAngle.x) * sin(mc.goalAngle.y);
		mc.goalPosition.z = mc.radius * cos(mc.goalAngle.x);

		dir = mc.goalPosition - tc.Position;
		normalized = glm::normalize(dir);
		float remainder = positionAdd - len;
		tc.Position += normalized * remainder;
	}
	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:

	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<AsteroidFieldMovementComponent> MovementComponents;

	ThreadComponentStorage<TransformComponent> ThreadTransformComponents;
	ThreadComponentStorage<AsteroidFieldMovementComponent> ThreadMovementComponents;

	ReadOnlySystemDataStorage<InputSystemData> InputSystemDataStorage;

	InputSystemData InputData;

	bool PauseToggled = false;
	bool Paused = false;
};
