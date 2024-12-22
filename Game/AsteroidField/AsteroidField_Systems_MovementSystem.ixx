module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module AsteroidField_Systems_MovementSystem;

import EE_Encosy_Entity;
import EE_Encosy_SystemThreaded;
import EE_ECS_Components_TransformComponent;
import AsteroidField_Components_MovementComponent;
import EE_Core_MatrixCalculations;
import EE_ECS_SystemData_InputSystem;

import <map>;
import <span>;
import <vector>;
import <random>;
import <algorithm>;
import <numeric>;


export class AsteroidFieldMovementSystem : public SystemThreaded
{

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

		EE_TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
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

	WriteReadComponentStorage<EE_TransformComponent> TransformComponents;
	WriteReadComponentStorage<AsteroidFieldMovementComponent> MovementComponents;

	ThreadComponentStorage<EE_TransformComponent> ThreadTransformComponents;
	ThreadComponentStorage<AsteroidFieldMovementComponent> ThreadMovementComponents;

	ReadOnlySystemDataStorage<EE_InputSystemData> InputSystemDataStorage;

	EE_InputSystemData InputData;

	bool PauseToggled = false;
	bool Paused = false;
};
