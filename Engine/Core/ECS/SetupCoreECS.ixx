module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module EncosyEngine.SetupCoreECS;

import ECS.Entity;
import ECS.EntityManager;
import ECS.ComponentManager;
import ECS.SystemManager;
import EncosyEngine.WindowManager;
import EncosyEngine.RenderCore;
import EncosyEngine.MatrixCalculations;

import Components.TransformComponent;
import Components.MovementComponent;
import Components.CameraComponent;
import Components.MaterialComponent;
import Components.TestReadOnlyComponent;

import Systems.CameraControllerSystem;
import Systems.InputSystem;
import Systems.MovementSystem;
import Systems.VulkanRenderSystem;

import SystemData.CameraControllerSystem;
import SystemData.InputSystem;

import <string>;
import <vector>;

export 
std::vector<EntityTypeInfo> InitializeEngineEntities(EntityManager* EM)
{
	std::vector<EntityTypeInfo> InitializedTypes;
		
	InitializedTypes.push_back(EM->CreateEntityType<TransformComponent, MovementComponent, CameraComponent,TestReadOnlyComponent>("CameraEntity"));
	InitializedTypes.push_back(EM->CreateEntityType<TransformComponent, MaterialComponent>("StaticSceneEntity"));
	InitializedTypes.push_back(EM->CreateEntityType<TransformComponent, MaterialComponent, MovementComponent>("DynamicSceneEntity"));

	return InitializedTypes;
}

export
void InitializeEngineSystems(EntityManager* EM, ComponentManager* CM, SystemManager* SM, WindowManager* WM, RenderCore* RC)
{


	// Input System
	InputSystemData inputSystemData = {};
	auto locator = CM->CreateSystemDataStorage(inputSystemData);
	auto system = SM->AddSystem<InputSystem>("InputSystem", true);
	WM->GetMainWindow()->SubscribeToEvents([=](SDL_Event e) {system->AddEventToQueue(e); });

	// Camera Controller System
	auto info = EM->GetEntityTypeInfo("CameraEntity");
	CameraControllerSystemData cameraSystemData =
	{
		.MainCamera = {},
		.Yaw = -90,
		.Pitch = 0,
	};

	locator = CM->CreateSystemDataStorage(cameraSystemData);
	SM->AddSystem<CameraControllerSystem>("CameraControllerSystem", true);

	// Movement System
	SM->AddSystem<MovementSystem>("MovementSystem", true);

	// Render System
	SM->AddSystem<VulkanRenderSystem>("VulkanRenderSystem", true, RC);
}

export
void CreateEngineEntities(EntityManager* EM, ComponentManager* CM, SystemManager* SM, WindowManager* WM, RenderCore* RC)
{
	float width = WM->GetMainWindow()->GetWidth();
	float height = WM->GetMainWindow()->GetHeight();
	float fovAngle = 90.0f;
	float near = 10000;
	float far = 0.1f;

	TransformComponent transform =
	{
		.Position = glm::vec3(0.0f, 0.0f, 5.0f),
		.Scale = glm::vec3(1.0f, 1.0f, 1.0f),
		.Orientation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f)
	};
	MovementComponent movement =
	{
		.Direction = glm::vec3(0.0f, 0.0f, 0.0f),
		.Speed = 2.0f
	};
	CameraComponent camera =
	{
		.Fov = fovAngle,
		.NearPlane = near,
		.FarPlane = far,
		.Front = glm::vec3(0.0f, 0.0f, 1.0f),
		.Right = glm::vec3(1.0f, 0.0f, 0.0f),
		.Up = glm::vec3(0.0f, 1.0f, 0.0f),
		.Orientation = MatrixCalculations::CalculateLookAtMatrix(transform.Position, transform.Position + camera.Front, camera.Up),
		.ProjectionMatrix = glm::mat4()
	};
	bool ortho = false;
	if (ortho)
	{
		camera.ProjectionMatrix = glm::ortho(-width / 2.0f, width / 2.0f, -height / 2.0f, height / 2.0f);
	}
	else
	{
		camera.ProjectionMatrix = glm::perspective(glm::radians(fovAngle), width / height, near, far);
	}

	TestReadOnlyComponent test = {};

	Entity createdCamera = EM->CreateEntityWithData(transform, movement, camera, test);
	CameraControllerSystemData* sd = CM->GetWriteReadSystemData<CameraControllerSystemData>();
	sd->MainCamera = createdCamera;

}
