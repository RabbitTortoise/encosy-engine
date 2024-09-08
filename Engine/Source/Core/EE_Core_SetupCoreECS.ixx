module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module EE_Core_SetupCoreECS;

import EE_Encosy_Entity;
import EE_Encosy_EntityManager;
import EE_Encosy_ComponentManager;
import EE_Encosy_SystemManager;
import EE_RenderCore;
import EE_Core_MatrixCalculations;
import EE_Core_WindowManager;

import EE_ECS_Components_TransformComponent;
import EE_ECS_Components_CameraComponent;
import EE_ECS_Components_MaterialComponent;

import EE_ECS_Systems_CameraControllerSystem;
import EE_ECS_Systems_InputSystem;
import EE_ECS_Systems_RaytracedRenderSystem;
import EE_ECS_Systems_ModelMatrixBuilderSystem;

import EE_ECS_SystemData_CameraControllerSystem;
import EE_ECS_SystemData_InputSystem;

import <string>;
import <vector>;

export
std::vector<EntityOperationResult> InitializeEngineEntities(EntityManager* EM)
{
	std::vector<EntityOperationResult> InitializedTypes;

	InitializedTypes.push_back(EM->CreateEntityType<EE_TransformComponent, EE_CameraComponent>("CameraEntity"));
	InitializedTypes.push_back(EM->CreateEntityType<EE_TransformComponent, EE_MaterialComponentRaytracing>("StaticSceneEntity"));

	return InitializedTypes;
}

export
void InitializeEngineSystems(EntityManager* EM, ComponentManager* CM, SystemManager* SM, RenderCore* RC)
{
	// Input System
	EE_InputSystemData inputSystemData = {};
	auto locator = CM->CreateSystemDataStorage(inputSystemData);
	auto system = SM->AddSystem<EE_InputSystem>("InputSystem");
	EncosyEngine::WindowManager::SubscribeToMainWindowEvents([=](SDL_Event e) { system->AddEventToQueue(e); });

	// Camera Controller System
	auto info = EM->GetEntityTypeInfo("CameraEntity");
	EE_CameraControllerSystemData cameraSystemData =
	{
		.MainCamera = {},
		.CurrentYaw = -90,
		.CurrentPitch = 0,
		.DesiredYaw = -90,
		.DesiredPitch = 0,
	};

	locator = CM->CreateSystemDataStorage(cameraSystemData);
	SM->AddSystem<EE_CameraControllerSystem>("CameraControllerSystem");

	// Raytraced Render System
	auto raytracingSystem = SM->AddSystem<EE_RaytracedRenderSystem>("RaytracedRenderSystem", RC);
}

export
void CreateEngineEntities(EntityManager* EM, ComponentManager* CM, SystemManager* SM, RenderCore* RC)
{
	float width = EncosyEngine::WindowManager::GetMainWindowWidth();
	float height = EncosyEngine::WindowManager::GetMainWindowHeight();
	float fovAngle = 90.0f;
	float near = 10000;
	float far = 0.1f;

	EE_TransformComponent transform =
	{
		.Position = glm::vec3(0.0f, 0.0f, 0.0f),
		.Scale = glm::vec3(1.0f, 1.0f, 1.0f),
		.Orientation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f)
	};
	EE_CameraComponent camera =
	{
		.Fov = fovAngle,
		.NearPlane = near,
		.FarPlane = far,
		.Front = glm::vec3(0.0f, 0.0f, 1.0f),
		.Right = glm::vec3(1.0f, 0.0f, 0.0f),
		.Up = glm::vec3(0.0f, 1.0f, 0.0f),
		.View = MatrixCalculations::CalculateLookAtMatrix(transform.Position, transform.Position + camera.Front, camera.Up),
		.Projection = glm::mat4()
	};
	bool ortho = false;
	if (ortho)
	{
		camera.Projection = glm::ortho(-width / 2.0f, width / 2.0f, -height / 2.0f, height / 2.0f);
	}
	else
	{
		camera.Projection = glm::perspective(glm::radians(fovAngle), width / height, near, far);
	}

	Entity createdCamera = EM->CreateEntityWithData(transform, camera);
	EE_CameraControllerSystemData& sd = CM->GetWriteReadSystemData<EE_CameraControllerSystemData>()[0];
	sd.MainCamera = createdCamera;

}
