module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module EncosyEngine.SetupCoreECS;

import EncosyCore.Entity;
import EncosyCore.EntityManager;
import EncosyCore.ComponentManager;
import EncosyCore.SystemManager;
import EncosyEngine.WindowManager;
import EncosyEngine.RenderCore;
import EncosyEngine.MatrixCalculations;

import Components.TransformComponent;
import Components.CameraComponent;
import Components.MaterialComponent;

import Systems.CameraControllerSystem;
import Systems.InputSystem;
import Systems.RaytracedRenderSystem;
import Systems.ModelMatrixBuilderSystem;

import SystemData.CameraControllerSystem;
import SystemData.InputSystem;

import <string>;
import <vector>;

export
std::vector<EntityOperationResult> InitializeEngineEntities(EntityManager* EM)
{
	std::vector<EntityOperationResult> InitializedTypes;

	InitializedTypes.push_back(EM->CreateEntityType<TransformComponent, CameraComponent>("CameraEntity"));
	InitializedTypes.push_back(EM->CreateEntityType<TransformComponent, MaterialComponentRaytracing>("StaticSceneEntity"));

	return InitializedTypes;
}

export
void InitializeEngineSystems(EntityManager* EM, ComponentManager* CM, SystemManager* SM, WindowManager* WM, RenderCore* RC)
{
	// Input System
	InputSystemData inputSystemData = {};
	auto locator = CM->CreateSystemDataStorage(inputSystemData);
	auto system = SM->AddSystem<InputSystem>("InputSystem");
	WM->GetMainWindow()->SubscribeToEvents([=](SDL_Event e) {system->AddEventToQueue(e); });

	// Camera Controller System
	auto info = EM->GetEntityTypeInfo("CameraEntity");
	CameraControllerSystemData cameraSystemData =
	{
		.MainCamera = {},
		.MainWindow = WM->GetMainWindow(),
		.CurrentYaw = -90,
		.CurrentPitch = 0,
		.DesiredYaw = -90,
		.DesiredPitch = 0,
	};

	locator = CM->CreateSystemDataStorage(cameraSystemData);
	SM->AddSystem<CameraControllerSystem>("CameraControllerSystem");

	// Raytraced Render System
	auto raytracingSystem = SM->AddSystem<RaytracedRenderSystem>("RaytracedRenderSystem", RC);
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
		.Position = glm::vec3(0.0f, 0.0f, 0.0f),
		.Scale = glm::vec3(1.0f, 1.0f, 1.0f),
		.Orientation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f)
	};
	CameraComponent camera =
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
	CameraControllerSystemData& sd = CM->GetWriteReadSystemData<CameraControllerSystemData>()[0];
	sd.MainCamera = createdCamera;

}
