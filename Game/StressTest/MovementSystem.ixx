module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module StressTest.Systems.MovementSystem;

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

export class MovementSystem : public System
{
	friend class SystemManager;

public:
	MovementSystem() {}
	~MovementSystem() {}

protected:
	void Init() override 
	{
		Type = SystemType::ThreadedSystem;
		SystemQueueIndex = 1100;

		auto cameraEntityInfo = WorldEntityManager->GetEntityTypeInfo("CameraEntity");
		cameraType = cameraEntityInfo.Type;
		AddWantedComponentDataForWriting(&TransformComponents);
		AddWantedComponentDataForWriting(&MovementComponents);

		AddAlwaysFetchedEntitiesForReading(cameraEntityInfo.Type, &CameraEntityComponents);
		AddSystemDataForReading(&InputSystemDataStorage);
		AddSystemDataForReading(&CameraControllerSystemDataStorage);

		AddForbiddenComponentType<CameraComponent>();
	}
	void PreUpdate(float deltaTime) override {}
	void Update(float deltaTime) override {}
	void UpdatePerEntity(float deltaTime, Entity entity, EntityType entityType) override
	{
		InputSystemData input = GetSystemData(&InputSystemDataStorage);
		CameraControllerSystemData csData = GetSystemData(&CameraControllerSystemDataStorage);
		CameraComponent mainCamera = GetEntityComponent(csData.MainCamera, cameraType, &CameraEntityComponents);

		TransformComponent& tc = GetCurrentEntityComponent(&TransformComponents);
		MovementComponent& mc = GetCurrentEntityComponent(&MovementComponents);

		if (input.Spacebar) { return; } // Continue only if spacebar is not pressed
		if (input.Return)
		{
			tc.Orientation = glm::quat_cast(mainCamera.View);
		}

		tc.Orientation = MatrixCalculations::RotateByWorldAxisX(tc.Orientation, mc.Speed * mc.Direction.x * deltaTime);
		tc.Orientation = MatrixCalculations::RotateByWorldAxisY(tc.Orientation, mc.Speed * mc.Direction.y * deltaTime);
		tc.Orientation = MatrixCalculations::RotateByWorldAxisZ(tc.Orientation, mc.Speed * mc.Direction.z * deltaTime);
	}

	void PostUpdate(float deltaTime) override {}
	void Destroy() override {}

private:

	EntityType cameraType = -1;

	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<MovementComponent> MovementComponents;

	ReadOnlyAlwaysFetchedStorage<CameraComponent> CameraEntityComponents;
	ReadOnlySystemDataStorage<InputSystemData> InputSystemDataStorage;
	ReadOnlySystemDataStorage<CameraControllerSystemData> CameraControllerSystemDataStorage;
};
