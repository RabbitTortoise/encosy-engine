module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module StressTest.Systems.MovementSystem;

import EncosyCore.Entity;
import EncosyCore.System;
import Components.TransformComponent;
import Components.StaticComponent;
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
		Type = SystemType::PhysicsSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;

		auto cameraEntityInfo = GetEntityTypeInfo("CameraEntity");
		cameraType = cameraEntityInfo.Type;
		AddComponentQueryForWriting(&TransformComponents);
		AddComponentQueryForWriting(&MovementComponents);

		AddEntitiesForReading(cameraEntityInfo.Type, &CameraEntityComponents);
		AddSystemDataForReading(&InputSystemDataStorage);
		AddSystemDataForReading(&CameraControllerSystemDataStorage);

		AddForbiddenComponentQuery<StaticComponent>();
		AddForbiddenComponentQuery<CameraComponent>();
	}
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override {}
	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override
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

	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:

	EntityType cameraType = -1;

	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<MovementComponent> MovementComponents;

	ReadOnlyAlwaysFetchedStorage<CameraComponent> CameraEntityComponents;
	ReadOnlySystemDataStorage<InputSystemData> InputSystemDataStorage;
	ReadOnlySystemDataStorage<CameraControllerSystemData> CameraControllerSystemDataStorage;
};
