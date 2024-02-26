module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module StressTest.Systems.MovementSystemThreaded;

import ECS.Entity;
import ECS.System;
import Components.TransformComponent;
import StressTest.Components.MovementComponent;
import Components.CameraComponent;
import EncosyEngine.MatrixCalculations;
import SystemData.InputSystem;
import SystemData.CameraControllerSystem;

import <map>;
import <span>;
import <vector>;
import <iostream>;


float RandomNumber0_1()
{
	return (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
}

export class MovementSystemThreaded : public ThreadedSystem
{
	friend class SystemManager;

public:
	MovementSystemThreaded() {}
	~MovementSystemThreaded() {}

protected:
	void Init() override
	{
		Type = SystemType::ThreadedSystem;
		SystemQueueIndex = 1100;

		auto cameraEntityInfo = WorldEntityManager->GetEntityTypeInfo("CameraEntity");
		cameraType = cameraEntityInfo.Type;
		AddWantedComponentDataForWriting(&TransformComponents, &ThreadTransformComponents);
		AddWantedComponentDataForWriting(&MovementComponents, &ThreadMovementComponents);

		AddAlwaysFetchedEntitiesForReading(cameraEntityInfo.Type, &CameraEntityComponents);
		AddSystemDataForReading(&InputSystemDataStorage);
		AddSystemDataForReading(&CameraControllerSystemDataStorage);

		AddForbiddenComponentType<CameraComponent>();
	}
	void PreUpdate(float deltaTime) override {}
	void Update(float deltaTime) override 
	{
		CameraControllerSystemData csData = GetSystemData(&CameraControllerSystemDataStorage);
		mainCamera = GetEntityComponent(csData.MainCamera, cameraType, &CameraEntityComponents);
	}
	void UpdatePerEntityThreaded(int thread, float deltaTime, Entity entity, EntityType entityType) override
	{
		InputSystemData input = GetSystemData(&InputSystemDataStorage);
		//

		TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		MovementComponent& mc = GetCurrentEntityComponent(thread, &ThreadMovementComponents);

		if (input.Spacebar) { return; } // Continue only if spacebar is not pressed
		if (input.Return)
		{
			tc.Orientation = glm::quat_cast(mainCamera.View);
			return;
		}

		tc.Orientation = MatrixCalculations::RotateByWorldAxisX(tc.Orientation, mc.Speed * mc.Direction.x * deltaTime);
		tc.Orientation = MatrixCalculations::RotateByWorldAxisY(tc.Orientation, mc.Speed * mc.Direction.y * deltaTime);
		tc.Orientation = MatrixCalculations::RotateByWorldAxisZ(tc.Orientation, mc.Speed * mc.Direction.z * deltaTime);
	}

	void PostUpdate(float deltaTime) override {}
	void Destroy() override {}

private:

	CameraComponent mainCamera;
	EntityType cameraType = -1;

	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<MovementComponent> MovementComponents;

	ReadOnlyAlwaysFetchedStorage<CameraComponent> CameraEntityComponents;
	ReadOnlySystemDataStorage<InputSystemData> InputSystemDataStorage;
	ReadOnlySystemDataStorage<CameraControllerSystemData> CameraControllerSystemDataStorage;

	ThreadComponentStorage<TransformComponent> ThreadTransformComponents;
	ThreadComponentStorage<MovementComponent> ThreadMovementComponents;
};
