module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


export module EncosyGame.Demo;

import EncosyEngine.Interface;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;

import Components.TransformComponent;
import Components.MaterialComponent;
import Components.ModelMatrixComponent;

import RenderCore.MeshLoader;
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


float RandomNumber0_1()
{
	return (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
}

glm::vec3 RandDir()
{
	return { RandomNumber0_1() + 0.01f,RandomNumber0_1() + 0.01f ,RandomNumber0_1() + 0.01f };
}

float RandSpeed()
{
	return (RandomNumber0_1() * 20.0f + 5.0f);
}

export void InitDemo()
{
	auto EngineCore = EncosyEngine::GetEncosyCore();
	auto PrimaryWorld = EngineCore->GetPrimaryWorld();

	auto WorldComponentManager = PrimaryWorld->GetWorldComponentManager();
	auto WorldEntityManager = PrimaryWorld->GetWorldEntityManager();
	auto WorldSystemManager = PrimaryWorld->GetWorldSystemManager();

	auto EngineRenderCore = EncosyEngine::GetRenderCore();
	auto MainTextureLoader = EngineRenderCore->GetTextureLoader();
	auto MainMeshLoader = EngineRenderCore->GetMeshLoader();
	auto MainRenderPipelineManager = EngineRenderCore->GetRenderPipelineManager();

	auto grassID = MainTextureLoader->LoadTexture("Grass_Texture.png");
	auto grassNormalID = MainTextureLoader->LoadTexture("Grass_Normal.png");
	auto rockID = MainTextureLoader->LoadTexture("Rock_Texture.png");
	auto rockNormalID = MainTextureLoader->LoadTexture("Rock_Normal.png");
	auto sandID = MainTextureLoader->LoadTexture("Sand_Texture.png");
	auto sandNormalID = MainTextureLoader->LoadTexture("Sand_Normal.png");
	auto snowID = MainTextureLoader->LoadTexture("Snow_Texture.png");
	auto snowNormalID = MainTextureLoader->LoadTexture("Snow_Normal.png");
	auto waterID = MainTextureLoader->LoadTexture("Water_Texture.png");
	auto waterNormalID = MainTextureLoader->LoadTexture("Water_Normal.png");

	std::vector<TextureID> textureIds;
	textureIds.push_back(grassID);
	textureIds.push_back(rockID);
	textureIds.push_back(sandID);
	textureIds.push_back(snowID);
	textureIds.push_back(waterID);

	std::vector<MeshID> meshIDs;
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Cube));
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere));
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Torus));

	ModelMatrixComponent matrix = {};
	TransformComponent tc = {};
	MaterialComponentLit mcLit = {};

	

}