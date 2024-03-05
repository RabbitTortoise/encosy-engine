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
import Components.CameraComponent;
import Components.MaterialComponent;

import Systems.CameraControllerSystem;
import Systems.InputSystem;
import Systems.UnlitRenderSystem;
import Systems.LitRenderSystem;
import Systems.LitRenderSystemThreaded;
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
	InitializedTypes.push_back(EM->CreateEntityType<TransformComponent, MaterialComponentUnlit>("StaticSceneEntityUnlit"));
	InitializedTypes.push_back(EM->CreateEntityType<TransformComponent, MaterialComponentLit>("StaticSceneEntityLit"));

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
		.Yaw = -90,
		.Pitch = 0,
	};

	locator = CM->CreateSystemDataStorage(cameraSystemData);
	SM->AddSystem<CameraControllerSystem>("CameraControllerSystem");

	// Model matrix builder
	SM->AddSystem<ModelMatrixBuilderSystem>("ModelMatrixBuilderSystem");
	// Render System
	SM->AddSystem<UnlitRenderSystem>("UnlitRenderSystem", RC);
	// Render System
	SM->AddSystem<LitRenderSystemThreaded>("LitRenderSystem", RC);
	//SM->AddSystem<LitRenderSystem>("LitRenderSystem", RC);
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
