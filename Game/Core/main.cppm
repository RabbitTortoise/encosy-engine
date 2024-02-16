module;

#include <locale.h>
#include <Windows.h>
#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EncosyGame;

import EncosyEngine.Interface;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;
import EncosyEngine.MatrixCalculations;
import Components.TransformComponent;
import Components.MovementComponent;
import Components.CameraComponent;
import Components.MaterialComponent;

import EncosyCore.ThreadedTaskRunner;
import RenderCore.MeshLoader;
import RenderCore.ShaderLoader;
import RenderCore.TextureLoader;
import RenderCore.RenderPipelineManager;

import <map>;
import <vector>;
import <string>;
import <random>;
import <iostream>;
import <chrono>;
import <thread>;
import <format>;
import <span>;
import <typeindex>;
import <typeinfo>;



void InitializeGameEntities()
{
	auto EngineCore = EncosyEngine::GetEncosyCore();
	auto PrimaryWorld = EngineCore->GetPrimaryWorld();

	auto WorldComponentManager = PrimaryWorld->GetWorldComponentManager();
	auto WorldEntityManager = PrimaryWorld->GetWorldEntityManager();
	auto WorldSystemManager = PrimaryWorld->GetWorldSystemManager();

	auto EngineRenderCore = EncosyEngine::GetRenderCore();
	auto MainTextureLoader = EngineRenderCore->GetTextureLoader();
	auto MainMeshLoader = EngineRenderCore->GetMeshLoader();
	auto MainShaderLoader = EngineRenderCore->GetShaderLoader();
	auto MainRenderPipelineManager = EngineRenderCore->GetRenderPipelineManager();


	auto grassID = MainTextureLoader->LoadTexture("Grass_Texture.png");
	auto rockID = MainTextureLoader->LoadTexture("Rock_Texture.png");
	auto sandID = MainTextureLoader->LoadTexture("Sand_Texture.png");
	auto snowID = MainTextureLoader->LoadTexture("Snow_Texture.png");
	auto waterID = MainTextureLoader->LoadTexture("Water_Texture.png");

	TransformComponent tc = {
	.Position = glm::vec3(0,-2,0),
	.Scale = glm::vec3(10,10,1),
	.Orientation = glm::quat(glm::vec3(glm::radians(90.0f),0,0)),
	};
	MaterialComponent mc = {
		.UsedRenderPipeline = MainRenderPipelineManager->GetEngineRenderPipelineID(EngineRenderPipelines::Unlit),
		.Texture_Albedo = grassID,
		.RenderMesh = MainMeshLoader->GetEngineMeshID(EngineMesh::Quad)
	};
	WorldEntityManager->CreateEntityWithData(tc, mc);

	mc = {
		.UsedRenderPipeline = MainRenderPipelineManager->GetEngineRenderPipelineID(EngineRenderPipelines::Unlit),
		.Texture_Albedo = rockID,
		.RenderMesh = MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere)
	};
	tc = {
	.Position = glm::vec3(-2,1,-2),
	.Scale = glm::vec3(.5,1,.5),
	.Orientation = glm::quat(glm::vec3(90,90,90)),
	};

	MovementComponent movc = {
		.Direction = glm::vec3(0,0,0),
		.Speed = 0.0f
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, movc);

	tc = {
	.Position = glm::vec3(2,1,2),
	.Scale = glm::vec3(.5,1,1),
	.Orientation = glm::quat(glm::vec3(90,0,0)),
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, movc);

	tc = {
	.Position = glm::vec3(-2,1,2),
	.Scale = glm::vec3(1,1,.5),
	.Orientation = glm::quat(glm::vec3(0,0,90)),
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, movc);

	tc = {
	.Position = glm::vec3(2,1,-2),
	.Scale = glm::vec3(1,.5f,1),
	.Orientation = glm::quat(glm::vec3(0,90,0)),
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, movc);

	tc = {
	.Position = glm::vec3(0,4,0),
	.Scale = glm::vec3(1,1,1),
	.Orientation = glm::quat(glm::vec3(0,0,0)),
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, movc);

}

export
int main()
{
	std::cout << "Started EncosyGame process" << std::endl;

	EncosyEngine::InitializeEngine();


	InitializeGameEntities();

	EncosyEngine::StartEngineLoop();

	std::cout << "Stopping EncosyGame process" << std::endl;
	return 0;
}
