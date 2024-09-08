module;

#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>
#include <fmt/core.h>

export module EE_ECS_Systems_CameraControllerSystem;

import EE_Encosy_Entity;
import EE_Encosy_System;
import EE_ECS_Components_TransformComponent;
import EE_ECS_Components_CameraComponent;
import EE_ECS_SystemData_CameraControllerSystem;
import EE_ECS_SystemData_InputSystem;
import EE_Core_MatrixCalculations;
import EE_Core_WindowManager;

import <map>;
import <span>;
import <vector>;
import <iostream>;


export
class EE_CameraControllerSystem : public System
{

public:
	EE_CameraControllerSystem() {}
	~EE_CameraControllerSystem() {}

	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;

		AddSystemDataForReading(&InputSystemDataStorage);
		AddSystemDataForWriting(&CameraSystemDataComponent);

		AddComponentQueryForWriting(&TransformComponents);
		AddComponentQueryForWriting(&CameraComponents);

	}

protected:
	
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override {}
	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override
	{

		EE_TransformComponent& transformComponent = GetCurrentEntityComponent(&TransformComponents);
		EE_CameraComponent& cameraComponent = GetCurrentEntityComponent(&CameraComponents);

		UpdateCamera(deltaTime, entity, transformComponent, cameraComponent);
	}

	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}


	void UpdateCamera(const double deltaTime, 
		Entity entity,
		EE_TransformComponent& transformComponent,
		EE_CameraComponent& cameraComponent)
	{
		EE_CameraControllerSystemData& controllerData = CameraSystemDataComponent.Storage[0];
		EE_InputSystemData inputData = InputSystemDataStorage.Storage[0];

		float rotateX = 0;
		float rotateY = 0;

		float rotateSpeed = 60.0f;


		if (inputData.MouseRightDown)
		{
			EncosyEngine::WindowManager::SetRelativeMouseModeForMainWindow(true);

			float totalX = (float)inputData.MouseRelativeMotion.x * rotateSpeed * deltaTime;
			float totalY = (float)inputData.MouseRelativeMotion.y * rotateSpeed * deltaTime;

			controllerData.DesiredYaw += totalX;
			controllerData.DesiredPitch -= totalY;

			if (controllerData.DesiredPitch > 89.0f)
				controllerData.DesiredPitch = 89.0f;
			if (controllerData.DesiredPitch < -89.0f)
				controllerData.DesiredPitch = -89.0f;
		}
		else
		{
			EncosyEngine::WindowManager::SetRelativeMouseModeForMainWindow(false);
		}

		glm::vec3 WorldUp = glm::vec3(0, 1, 0);
		glm::vec3 front;

		glm::vec2 dir = 
			glm::vec2(controllerData.DesiredYaw, controllerData.DesiredPitch) - 
			glm::vec2(controllerData.CurrentYaw, controllerData.CurrentPitch);
		float len = glm::length(dir);

		float yawAdd = 0.0f;
		float pitchAdd = 0.0f;
		float clamp = 0.01f;
		float smoothing = 0.2f;

		if (len != 0)
		{
			glm::vec2 normalized = glm::normalize(dir);
			yawAdd = normalized.x * len * (1 - smoothing);
			pitchAdd = normalized.y * len * (1 - smoothing);
			if (std::abs(yawAdd) > std::abs(dir.x))
			{
				yawAdd = dir.x;
			}
			if (std::abs(pitchAdd) > std::abs(dir.y))
			{
				pitchAdd = dir.y;
			}
		}
		
		if (std::abs(controllerData.DesiredYaw - controllerData.CurrentYaw) <= clamp)
		{
			controllerData.CurrentYaw = controllerData.DesiredYaw;
		}
		else
		{
			controllerData.CurrentYaw += yawAdd;
		}
		if (std::abs(controllerData.DesiredPitch - controllerData.CurrentPitch) <= clamp)
		{
			controllerData.CurrentPitch = controllerData.DesiredPitch;
		}
		else
		{
			controllerData.CurrentPitch += pitchAdd;
		}

		float Yaw = controllerData.CurrentYaw;
		float Pitch = controllerData.CurrentPitch;
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

			transformComponent.Position += direction * static_cast<float>(speed * speedMultiplier * deltaTime);
			cameraComponent.View = MatrixCalculations::CalculateLookAtMatrix(transformComponent.Position, transformComponent.Position + cameraComponent.Front, cameraComponent.Up);

			//fmt::println("POS:  {},{},{}", transformComponent.Position.x, transformComponent.Position.y, transformComponent.Position.z);
		}
	}
	
private:
	WriteReadComponentStorage<EE_TransformComponent> TransformComponents;
	WriteReadComponentStorage<EE_CameraComponent> CameraComponents;

	WriteReadSystemDataStorage<EE_CameraControllerSystemData> CameraSystemDataComponent;
	ReadOnlySystemDataStorage<EE_InputSystemData> InputSystemDataStorage;

};
