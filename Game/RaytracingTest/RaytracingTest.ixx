module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EncosyGame.RaytracingTest;

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
import RenderCore.VulkanTypes;

import EncosyGame.DemoCommon;

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


void CreateAsteroidFieldTextures(TextureLoader* MainTextureLoader, RenderCore* EngineRenderCore, std::vector<PBRTextureSet>& sets, std::vector<TextureSetID>& setIds)
{
	//auto whiteID = MainTextureLoader->GetEngineTextureID(EngineTextures::White);
	auto albedo = MainTextureLoader->LoadTexture("angled-shale-cliff/angled-shale-cliff_albedo.png");
	auto ao = MainTextureLoader->LoadTexture("angled-shale-cliff/angled-shale-cliff_ao.png");
	auto depth = MainTextureLoader->LoadTexture("angled-shale-cliff/angled-shale-cliff_depth.png");
	auto metallic = MainTextureLoader->LoadTexture("angled-shale-cliff/angled-shale-cliff_metallic.png");
	auto normal = MainTextureLoader->LoadTexture("angled-shale-cliff/angled-shale-cliff_normal.png");
	auto rouhgness = MainTextureLoader->LoadTexture("angled-shale-cliff/angled-shale-cliff_roughness.png");

	PBRTextureSet set = PBRTextureSet(albedo, ao, depth, metallic, normal, rouhgness);
	auto id = EngineRenderCore->RegisterTextureSetForRaytracingUsage(set);
	sets.push_back(set);
	setIds.push_back(id);

	albedo = MainTextureLoader->LoadTexture("layered-rock/layered-rock_albedo.png");
	ao = MainTextureLoader->LoadTexture("layered-rock/layered-rock_ao.png");
	depth = MainTextureLoader->LoadTexture("layered-rock/layered-rock_depth.png");
	metallic = MainTextureLoader->LoadTexture("layered-rock/layered-rock_metallic.png");
	normal = MainTextureLoader->LoadTexture("layered-rock/layered-rock_normal.png");
	rouhgness = MainTextureLoader->LoadTexture("layered-rock/layered-rock_roughness.png");

	set = PBRTextureSet(albedo, ao, depth, metallic, normal, rouhgness);
	id = EngineRenderCore->RegisterTextureSetForRaytracingUsage(set);
	sets.push_back(set);
	setIds.push_back(id);

	albedo = MainTextureLoader->LoadTexture("jagged-rockface/jagged-rockface_albedo.png");
	ao = MainTextureLoader->LoadTexture("jagged-rockface/jagged-rockface_ao.png");
	depth = MainTextureLoader->LoadTexture("jagged-rockface/jagged-rockface_depth.png");
	metallic = MainTextureLoader->LoadTexture("jagged-rockface/jagged-rockface_metallic.png");
	normal = MainTextureLoader->LoadTexture("jagged-rockface/jagged-rockface_normal.png");
	rouhgness = MainTextureLoader->LoadTexture("jagged-rockface/jagged-rockface_roughness.png");

	set = PBRTextureSet(albedo, ao, depth, metallic, normal, rouhgness);
	id = EngineRenderCore->RegisterTextureSetForRaytracingUsage(set);
	sets.push_back(set);
	setIds.push_back(id);

	albedo = MainTextureLoader->LoadTexture("red-coral/red-coral_albedo.png");
	ao = MainTextureLoader->LoadTexture("red-coral/red-coral_ao.png");
	depth = MainTextureLoader->LoadTexture("red-coral/red-coral_depth.png");
	metallic = MainTextureLoader->LoadTexture("red-coral/red-coral_metallic.png");
	normal = MainTextureLoader->LoadTexture("red-coral/red-coral_normal.png");
	rouhgness = MainTextureLoader->LoadTexture("red-coral/red-coral_roughness.png");

	set = PBRTextureSet(albedo, ao, depth, metallic, normal, rouhgness);
	id = EngineRenderCore->RegisterTextureSetForRaytracingUsage(set);
	sets.push_back(set);
	setIds.push_back(id);

	//albedo = MainTextureLoader->LoadTexture("ridged-foam/ridged-foam_albedo.png");
	//ao = MainTextureLoader->LoadTexture("ridged-foam/ridged-foam_ao.png");
	//depth = MainTextureLoader->LoadTexture("ridged-foam/ridged-foam_depth.png");
	//metallic = MainTextureLoader->LoadTexture("ridged-foam/ridged-foam_metallic.png");
	//normal = MainTextureLoader->LoadTexture("ridged-foam/ridged-foam_normal.png");
	//rouhgness = MainTextureLoader->LoadTexture("ridged-foam/ridged-foam_roughness.png");

	//set = PBRTextureSet(albedo, ao, depth, metallic, normal, rouhgness);
	//id = EngineRenderCore->RegisterTextureSetForRaytracingUsage(set);
	//sets.push_back(set);
	//setIds.push_back(id);

	albedo = MainTextureLoader->LoadTexture("snowcoveredpath/snowcoveredpath_albedo.png");
	ao = MainTextureLoader->LoadTexture("snowcoveredpath/snowcoveredpath_ao.png");
	depth = MainTextureLoader->LoadTexture("snowcoveredpath/snowcoveredpath_depth.png");
	metallic = MainTextureLoader->LoadTexture("snowcoveredpath/snowcoveredpath_metallic.png");
	normal = MainTextureLoader->LoadTexture("snowcoveredpath/snowcoveredpath_normal.png");
	rouhgness = MainTextureLoader->LoadTexture("snowcoveredpath/snowcoveredpath_roughness.png");

	set = PBRTextureSet(albedo, ao, depth, metallic, normal, rouhgness);
	id = EngineRenderCore->RegisterTextureSetForRaytracingUsage(set);
	sets.push_back(set);
	setIds.push_back(id);

	albedo = MainTextureLoader->LoadTexture("vines/vines_albedo.png");
	ao = MainTextureLoader->LoadTexture("vines/vines_ao.png");
	depth = MainTextureLoader->LoadTexture("vines/vines_depth.png");
	metallic = MainTextureLoader->LoadTexture("vines/vines_metallic.png");
	normal = MainTextureLoader->LoadTexture("vines/vines_normal.png");
	rouhgness = MainTextureLoader->LoadTexture("vines/vines_roughness.png");

	set = PBRTextureSet(albedo, ao, depth, metallic, normal, rouhgness);
	id = EngineRenderCore->RegisterTextureSetForRaytracingUsage(set);
	sets.push_back(set);
	setIds.push_back(id);

}



export void InitRaytracingTest(int testDimensionsX, int testDimensionsY, int testDimensionsZ)
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

	RenderCoreResources* CoreResources = EngineRenderCore->GetRenderCoreResources();

	// Models
	auto rock = MainMeshLoader->GetMeshID("Rock.obj");
	auto rock2 = MainMeshLoader->GetMeshID("Rock2.obj");
	auto rock3 = MainMeshLoader->GetMeshID("Rock3.obj");
	auto rock4 = MainMeshLoader->GetMeshID("Rock4.obj");

	// Textures
	std::vector<PBRTextureSet> textureSets;
	std::vector<TextureSetID> textureSetIDs;
	CreateAsteroidFieldTextures(MainTextureLoader, EngineRenderCore, textureSets, textureSetIDs);

	EngineRenderCore->RegisterMeshForRaytracingUsage(rock);
	EngineRenderCore->RegisterMeshForRaytracingUsage(rock2);
	EngineRenderCore->RegisterMeshForRaytracingUsage(rock3);
	EngineRenderCore->RegisterMeshForRaytracingUsage(rock4);

	std::vector<MeshID> meshIDs;
	//meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Cube));
	//meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere));
	//meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Torus));
	meshIDs.push_back(rock);
	meshIDs.push_back(rock2);
	meshIDs.push_back(rock3);
	meshIDs.push_back(rock4);

	//PaletteInfo palette = CreatePaletteArray64();
	//CoreResources->RenderPaletteInfo = palette;


	// Movement System
	WorldSystemManager->AddSystem<MovementSystemThreaded>("MovementSystem");

	ModelMatrixComponent matrix = {};
	TransformComponent tc = {};
	MovementComponent movc = {};
	MaterialComponentRaytracing mcRay = {};

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
				glm::vec3 dir = RandDir();
				float speed = RandSpeed();

				float rand1 = RandomNumber0_1();
				float rand2 = RandomNumber0_1();
				float rand3 = RandomNumber0_1();
				int textureSelect = std::round(rand1 * (textureSets.size() - 1)) + 1;
				int meshSelect = std::round(rand2 * (meshIDs.size() - 1));

				MeshID usedMeshId = meshIDs[meshSelect];
				tc = {
					.Position = glm::vec3(xCur,yCur,zCur),
					.Scale = glm::vec3(0.175,0.175,0.175),
					.Orientation = glm::quat(glm::vec3(0,0,0)),
				};
				movc = {
					.Direction = dir,
					.Speed = speed
				};
				mcRay.TextureSet = textureSelect;
				mcRay.RenderMesh = usedMeshId;
				mcRay.TextureRepeat = 1.0f;
				mcRay.Color = glm::vec3(1, 1, 1);
			
				WorldEntityManager->CreateEntityWithData(tc, movc, matrix, mcRay);

				zCur -= dist;
			}
			yCur += dist;
		}
		xCur += dist;
	}
}
