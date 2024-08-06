module;

#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EncosyTesting;

import EncosyEngine.Interface;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;
import EncosyEngine.MatrixCalculations;
import Components.TransformComponent;
import Components.CameraComponent;
import Components.MaterialComponent;
import Components.ModelMatrixComponent;

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

void Tests()
{
	auto EngineCore = EncosyEngine::GetEncosyCore();
	auto PrimaryWorld = EngineCore->GetPrimaryWorld();

	auto WorldComponentManager = PrimaryWorld->GetWorldComponentManager();
	auto WorldEntityManager = PrimaryWorld->GetWorldEntityManager();
	auto WorldSystemManager = PrimaryWorld->GetWorldSystemManager();

}

void InitializeTestEntities()
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


	auto error = MainTextureLoader->GetEngineTextureID(EngineTextures::ErrorCheckerBoard);
	auto whiteID = MainTextureLoader->GetEngineTextureID(EngineTextures::White);
	auto blackID = MainTextureLoader->GetEngineTextureID(EngineTextures::Black);
	auto greyID = MainTextureLoader->GetEngineTextureID(EngineTextures::Grey);
	auto normalID = MainTextureLoader->GetEngineTextureID(EngineTextures::NeutralNormal);
	PBRTextureSet textureSet = PBRTextureSet(error, whiteID, greyID, blackID, normalID, greyID);
	auto textureSetID = EngineRenderCore->RegisterTextureSetForRaytracingUsage(textureSet);


	TransformComponent tc = {
	.Position = glm::vec3(0,-2,0),
	.Scale = glm::vec3(10,1,10),
	.Orientation = glm::quat(glm::vec3(glm::radians(0.0f),0,0)),
	};

	MaterialComponentRaytracing mc = {};
	mc.TextureSet = textureSetID;
	mc.RenderMesh = MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere);
	mc.TextureRepeat = 1.0f;
	mc.Color = glm::vec3(1, 1, 1);

	ModelMatrixComponent model = {};

	WorldEntityManager->CreateEntityWithData(tc, mc, model);

	tc = {
	.Position = glm::vec3(-2,1,-2),
	.Scale = glm::vec3(.5,1,.5),
	.Orientation = glm::quat(glm::vec3(90,90,90)),
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, model);

	tc = {
	.Position = glm::vec3(2,1,2),
	.Scale = glm::vec3(.5,1,1),
	.Orientation = glm::quat(glm::vec3(90,0,0)),
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, model);

	tc = {
	.Position = glm::vec3(-2,1,2),
	.Scale = glm::vec3(1,1,.5),
	.Orientation = glm::quat(glm::vec3(0,0,90)),
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, model);

	tc = {
	.Position = glm::vec3(2,1,-2),
	.Scale = glm::vec3(1,.5f,1),
	.Orientation = glm::quat(glm::vec3(0,90,0)),
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, model);

	tc = {
	.Position = glm::vec3(0,4,0),
	.Scale = glm::vec3(1,1,1),
	.Orientation = glm::quat(glm::vec3(0,0,0)),
	};

	WorldEntityManager->CreateEntityWithData(tc, mc, model);

}

export
int main()
{
    std::cout << "Started EncosyEngine test process" << std::endl;
    
	EncosyEngine::InitializeEngine();

	InitializeTestEntities();

	EncosyEngine::StartEngineLoop();
	
    std::cout << "Stopping EncosyEngine test process" << std::endl;
    return 0;
}
