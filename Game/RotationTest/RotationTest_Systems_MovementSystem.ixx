module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module RotationTest_Systems_MovementSystem;

import EE_Encosy_Entity;
import EE_Encosy_System;
import EE_ECS_Components_TransformComponent;
import EE_ECS_Components_StaticComponent;
import RotationTest_Components_MovementComponent.ixx;
import EE_ECS_Components_CameraComponent;
import EE_Core_MatrixCalculations;
import EE_ECS_SystemData_InputSystem;
import EE_ECS_SystemData_CameraControllerSystem;

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

public:
	MovementSystem() {}
	~MovementSystem() {}


	void Init() override 
	{
		Type = SystemType::PhysicsSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;

		auto cameraEntityInfo = GetEntityTypeInfo("CameraEntity");
		cameraType = cameraEntityInfo.Type;
		AddComponentQueryForWriting(&TransformComponents);
		AddComponentQueryForWriting(&MovementComponents);

		AddEntitiesForReading(cameraEntityInfo.Type, &CameraEntityComponents);
		AddSystemDataForReading(&InputSystemDataStorage);
		AddSystemDataForReading(&CameraControllerSystemDataStorage);

		AddForbiddenComponentQuery<EE_StaticComponent>();
		AddForbiddenComponentQuery<EE_CameraComponent>();
	}
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override
	{
		EE_CameraControllerSystemData csData = GetSystemData(&CameraControllerSystemDataStorage);
		mainCamera = GetEntityComponent(csData.MainCamera, cameraType, &CameraEntityComponents);
	}
	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override
	{
		EE_InputSystemData input = GetSystemData(&InputSystemDataStorage);
		EE_TransformComponent& tc = GetCurrentEntityComponent(&TransformComponents);
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

	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:

	EE_CameraComponent mainCamera;
	EntityType cameraType = -1;

	WriteReadComponentStorage<EE_TransformComponent> TransformComponents;
	WriteReadComponentStorage<MovementComponent> MovementComponents;

	ReadOnlyAlwaysFetchedStorage<EE_CameraComponent> CameraEntityComponents;
	ReadOnlySystemDataStorage<EE_InputSystemData> InputSystemDataStorage;
	ReadOnlySystemDataStorage<EE_CameraControllerSystemData> CameraControllerSystemDataStorage;
};
