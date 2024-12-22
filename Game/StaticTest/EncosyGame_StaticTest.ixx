module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EncosyGame_StaticTest;

import EncosyEngine_EngineCore;
import EncosyEngine_EncosyCore;
import EncosyEngine_RenderCore;
import EE_Core_MatrixCalculations;

import EE_ECS_Components_TransformComponent;
import EE_ECS_Components_MaterialComponent;
import EE_ECS_Components_ModelMatrixComponent;
import EE_ECS_Components_StaticComponent;

import EE_RenderCore_MeshLoader;
import EE_RenderCore_TextureLoader;
import EE_RenderCore_RenderPipelineManager;
import EE_RenderCore_VulkanTypes;

import EncosyGame_DemoCommon;

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


export void InitStaticTest(int testDimensionsX, int testDimensionsY, int testDimensionsZ)
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

	// Textures
	std::vector<PBRTextureSet> textureSets;
	std::vector<TextureSetID> textureSetIDs;
	CreateTextures(MainTextureLoader, EngineRenderCore, textureSets, textureSetIDs);

	std::vector<MeshID> meshIDs;
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Cube));
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere));
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Torus));

	EE_TransformComponent tc = {};
	EE_StaticComponent stat = {};
	EE_MaterialComponent mcRay = {};

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
			for (size_t z = 0; z < testDimensionsZ; z++)
			{

				float rand1 = RandomNumber0_1();
				float rand2 = RandomNumber0_1();
				int textureSelect = std::round(rand1 * (textureSets.size() - 1)) + 1;
				int meshSelect = std::round(rand2 * (meshIDs.size() - 1));

				MeshID usedMeshId = meshIDs[meshSelect];
				tc = {
					.Position = glm::vec3(xCur,yCur,zCur),
					.Scale = glm::vec3(0.75,0.75,0.75),
					.Orientation = glm::quat(glm::vec3(0,0,0)),
				};

				mcRay.TextureSet = textureSelect;
				mcRay.RenderMesh = usedMeshId;
				mcRay.TextureRepeat = 1.0f;
				mcRay.Color = glm::vec3(1, 1, 1);

				WorldEntityManager->CreateEntityWithData(tc, mcRay, stat);

				zCur -= dist;
			}
			yCur += dist;
		}
		xCur += dist;
	}

}
