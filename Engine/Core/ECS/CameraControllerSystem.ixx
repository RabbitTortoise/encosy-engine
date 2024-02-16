module;

#include <glm/glm.hpp>	
#include <fmt/core.h>

export module Systems.CameraControllerSystem;

import ECS.Entity;
import ECS.System;
import Components.TransformComponent;
import Components.MovementComponent;
import Components.CameraComponent;
import Components.TestReadOnlyComponent;
import SystemData.CameraControllerSystem;
import SystemData.InputSystem;
import EncosyEngine.MatrixCalculations;

import <map>;
import <span>;
import <vector>;
import <iostream>;

export
class CameraControllerSystem : public System
{
	friend class SystemManager;

public:
	CameraControllerSystem() {}
	~CameraControllerSystem() {}

protected:
	void Init() override 
	{
		Type = SystemType::System;
		SystemQueueIndex = 1000;

		AddSystemDataForReading(&InputSystemDataComponent);
		AddSystemDataForWriting(CameraSystemDataComponent);

		AddWantedComponentDataForWriting(
			&TransformComponents,
			&MovementComponents,
			&CameraComponents
		);
	}
	void PreUpdate(float deltaTime) override {}
	void Update(float deltaTime) override {}
	void UpdatePerEntity(float deltaTime, Entity entity, size_t vectorIndex, size_t spanIndex) override
	{
		TransformComponent& transformComponent = TransformComponents[vectorIndex][spanIndex];
		MovementComponent& movementComponent = MovementComponents[vectorIndex][spanIndex];
		CameraComponent& cameraComponent = CameraComponents[vectorIndex][spanIndex];

		UpdateCamera(deltaTime, entity, transformComponent, movementComponent, cameraComponent);
	}

	void PostUpdate(float deltaTime) override {}
	void Destroy() override {}


	void UpdateCamera(float deltaTime, 
		Entity entity,
		TransformComponent& transformComponent, 
		MovementComponent& movementComponent,
		CameraComponent& cameraComponent)
	{
		CameraControllerSystemData& controllerData = *CameraSystemDataComponent;
		const InputSystemData& inputData = InputSystemDataComponent;

		if (inputData.MouseRightDown)
		{
			
			controllerData.Yaw += inputData.MouseRelativeMotion.x / 2.0f;
			controllerData.Pitch -= inputData.MouseRelativeMotion.y / 2.0f;

			if (controllerData.Pitch > 89.0f)
				controllerData.Pitch = 89.0f;
			if (controllerData.Pitch < -89.0f)
				controllerData.Pitch = -89.0f;
		}

		glm::vec3 WorldUp = glm::vec3(0, 1, 0);
		glm::vec3 front;
		float Yaw = controllerData.Yaw;
		float Pitch = controllerData.Pitch;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		cameraComponent.Front = glm::normalize(front);
		cameraComponent.Right = glm::normalize(glm::cross(cameraComponent.Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		cameraComponent.Up = glm::normalize(glm::cross(cameraComponent.Right, cameraComponent.Front));

		//Debug Camera Movement
		if (entity.GetID() == controllerData.MainCamera.GetID())
		{
			movementComponent.Direction = glm::vec3(0, 0, 0);
			float speedMultiplier = 1.0f;

				
			if (inputData.W) { movementComponent.Direction += cameraComponent.Front; }
			if (inputData.S) { movementComponent.Direction += -cameraComponent.Front; }
			if (inputData.A) { movementComponent.Direction += -cameraComponent.Right; }
			if (inputData.D) { movementComponent.Direction += cameraComponent.Right; }
			if (inputData.Q) { movementComponent.Direction += WorldUp; }
			if (inputData.E) { movementComponent.Direction += -WorldUp; }
			if (inputData.Left_Shift) { speedMultiplier += 2.0f; }
			if (inputData.Left_Control) { speedMultiplier -= 0.5f; }
			glm::normalize(movementComponent.Direction);

			transformComponent.Position += movementComponent.Direction * movementComponent.Speed * speedMultiplier * deltaTime;

			cameraComponent.Orientation = MatrixCalculations::CalculateLookAtMatrix(transformComponent.Position, transformComponent.Position + cameraComponent.Front, cameraComponent.Up);

			//fmt::println("POS:  {},{},{}", transformComponent.Position.x, transformComponent.Position.y, transformComponent.Position.z);
		}
	}

	//int CreateCamera(int width, int height, bool ortho = true, float fovAngle = 45.0f, float near = 0.1f, float far = 100.0f);
	//void UpdateCameraProjectionMatrix(CameraEntity* camera, int width, int height, bool ortho = true);
	//void SetMainCamera(int cameraID);
	//CameraEntity* GetMainCamera();
	
private:

	std::vector<std::span<TransformComponent>> TransformComponents;
	std::vector<std::span<MovementComponent>> MovementComponents;
	std::vector<std::span<CameraComponent>> CameraComponents;

	CameraControllerSystemData* CameraSystemDataComponent;
	InputSystemData InputSystemDataComponent;
};
