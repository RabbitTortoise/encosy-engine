module;

#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>
#include <fmt/core.h>

export module Systems.CameraControllerSystem;

import ECS.Entity;
import ECS.System;
import Components.TransformComponent;
import Components.CameraComponent;
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

		AddSystemDataForReading(&InputSystemDataStorage);
		AddSystemDataForWriting(&CameraSystemDataComponent);

		AddWantedComponentDataForWriting(&TransformComponents);
		AddWantedComponentDataForWriting(&CameraComponents);

	}
	void PreUpdate(float deltaTime) override {}
	void Update(float deltaTime) override {}
	void UpdatePerEntity(float deltaTime, Entity entity, EntityType entityType) override
	{

		TransformComponent& transformComponent = GetCurrentEntityComponent(&TransformComponents);
		CameraComponent& cameraComponent = GetCurrentEntityComponent(&CameraComponents);

		UpdateCamera(deltaTime, entity, transformComponent, cameraComponent);
	}

	void PostUpdate(float deltaTime) override {}
	void Destroy() override {}


	void UpdateCamera(float deltaTime, 
		Entity entity,
		TransformComponent& transformComponent, 
		CameraComponent& cameraComponent)
	{
		CameraControllerSystemData& controllerData = CameraSystemDataComponent.Storage[0];
		InputSystemData inputData = InputSystemDataStorage.Storage[0];

		float rotateX = 0;
		float rotateY = 0;

		if (inputData.MouseRightDown)
		{
			controllerData.MainWindow->SetRelativeMouseMode(true);
			controllerData.Yaw += (float)inputData.MouseRelativeMotion.x * 60.0f * deltaTime;
			controllerData.Pitch -= (float)inputData.MouseRelativeMotion.y * 60.0f * deltaTime;

			if (controllerData.Pitch > 89.0f)
				controllerData.Pitch = 89.0f;
			if (controllerData.Pitch < -89.0f)
				controllerData.Pitch = -89.0f;

			//rotateX = (float)inputData.MouseRelativeMotion.x * 50.0f * deltaTime;
			//rotateY = (float)inputData.MouseRelativeMotion.y * 50.0f * deltaTime;
		}
		else
		{
			controllerData.MainWindow->SetRelativeMouseMode(false);
		}


		//auto orientation = transformComponent.Orientation;
		//orientation = MatrixCalculations::RotateByLocalAxisY(orientation, glm::radians(rotateX));
		//orientation = MatrixCalculations::RotateByLocalAxisX(orientation, glm::radians(-rotateY));
		//transformComponent.Orientation = orientation;

		glm::vec3 WorldUp = glm::vec3(0, 1, 0);
		//glm::vec3 front = orientation * glm::vec3(0, 0, 1);
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
		if (entity == controllerData.MainCamera)
		{
			auto direction = glm::vec3(0, 0, 0);
			float speed = 2.5f;
			float speedMultiplier = 1.0f;

				
			if (inputData.W) { direction += cameraComponent.Front; }
			if (inputData.S) { direction += -cameraComponent.Front; }
			if (inputData.A) { direction += -cameraComponent.Right; }
			if (inputData.D) { direction += cameraComponent.Right; }
			if (inputData.Q) { direction += WorldUp; }
			if (inputData.E) { direction += -WorldUp; }
			if (inputData.Left_Shift) { speedMultiplier += 2.0f; }
			if (inputData.Left_Control) { speedMultiplier -= 0.25f; }
			glm::normalize(direction);

			transformComponent.Position += direction * speed * speedMultiplier * deltaTime;
			cameraComponent.View = MatrixCalculations::CalculateLookAtMatrix(transformComponent.Position, transformComponent.Position + cameraComponent.Front, cameraComponent.Up);

			//fmt::println("POS:  {},{},{}", transformComponent.Position.x, transformComponent.Position.y, transformComponent.Position.z);
		}
	}
	
private:
	WriteReadComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<CameraComponent> CameraComponents;

	WriteReadSystemDataStorage<CameraControllerSystemData> CameraSystemDataComponent;
	ReadOnlySystemDataStorage<InputSystemData> InputSystemDataStorage;

};
