module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EncosyGame.RotationTest;

import EncosyEngine.Interface;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;

import Components.TransformComponent;
import Components.MaterialComponent;
import Components.ModelMatrixComponent;

import StressTest.Components.MovementComponent;
import StressTest.Systems.MovementSystem;
import StressTest.Systems.MovementSystemThreaded;

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


export void InitRotationTest(int testDimensionsX, int testDimensionsY, int testDimensionsZ)
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


	// Movement System
	WorldSystemManager->AddSystem<MovementSystemThreaded>("MovementSystem");

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

	std::vector<MeshID> meshIDs;
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Cube));
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere));
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Torus));

	std::vector<TextureID> textureIds;
	textureIds.push_back(grassID);
	textureIds.push_back(rockID);
	textureIds.push_back(sandID);
	textureIds.push_back(snowID);
	textureIds.push_back(waterID);

	ModelMatrixComponent matrix = {};
	TransformComponent tc = {};
	MaterialComponentLit mcLit = {};
	MovementComponent movc = {};

	float dist = 2.0f;

	int xStart = (-testDimensionsX) * dist / 2.0f;
	int yStart = (-testDimensionsY) * dist / 2.0f;
	int zStart = -15;
	int xCur = xStart;
	int yCur = yStart;
	int zCur = zStart;

	for (size_t x = 0; x < testDimensionsX; x++)
	{
		yCur = yStart;
		for (size_t y = 0; y < testDimensionsY; y++)
		{
			zCur = zStart;
			for (size_t z = 0; z < testDimensionsY; z++)
			{
				glm::vec3 dir = RandDir();
				float speed = RandSpeed();

				float rand1 = RandomNumber0_1();
				float rand2 = RandomNumber0_1();
				int textureSelect = std::round(rand1 * (textureIds.size() - 1));
				int meshSelect = std::round(rand2 * (meshIDs.size() - 1));

				TextureID usedTextureID = textureIds[textureSelect];
				MeshID usedMeshId = meshIDs[meshSelect];
				mcLit = {
					.Diffuse = usedTextureID,
					.Normal = usedTextureID + 1,
					.RenderMesh = usedMeshId,
					.TextureRepeat = 1.0f
				};
				tc = {
					.Position = glm::vec3(xCur,yCur,zCur),
					.Scale = glm::vec3(0.75,0.75,0.75),
					.Orientation = glm::quat(glm::vec3(0,0,0)),
				};
				movc = {
					.Direction = dir,
					.Speed = speed
				};

				WorldEntityManager->CreateEntityWithData(tc, mcLit, movc, matrix);

				zCur -= dist;
			}
			yCur += dist;
		}
		xCur += dist;
	}

}
