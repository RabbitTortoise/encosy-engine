module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EncosyGame.AsteroidField;

import EncosyEngine.Interface;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;

import Components.TransformComponent;
import Components.MaterialComponent;
import Components.ModelMatrixComponent;

import AsteroidField.Systems.MovementSystem;
import AsteroidField.Components.MovementComponent;

import RenderCore.MeshLoader;
import RenderCore.TextureLoader;
import RenderCore.RenderPipelineManager;
import RenderCore.VulkanTypes;
import RenderCore.Resources;

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
import <random>;


glm::vec3 CalculateGoalPosition(float radius, float goalAngleX, float goalAngleY)
{
	glm::vec3 pos = glm::vec3(0);
	pos.x = radius * sin(goalAngleX) * cos(goalAngleY);
	pos.y = radius * sin(goalAngleX) * sin(goalAngleY);
	pos.z = radius * cos(goalAngleX);
	return pos;
}


void CreateAsteroidFieldTextures(TextureLoader* MainTextureLoader, RenderCore* EngineRenderCore, std::vector<PBRTextureSet>& sets, std::vector<TextureSetID>& setIds)
{
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

export void InitAsteroidField(unsigned int asteroidCount)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distr01(0.0f, 1.0f);

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

	// Lighting settings
	CoreResources->BackgroundGradientData.data1 = glm::vec4(0);
	CoreResources->BackgroundGradientData.data2 = glm::vec4(0);
	CoreResources->GlobalLightingData.directionalLightDir = glm::vec3(0.001f, 0.001f, 0.001f);

	// Movement System
	WorldSystemManager->AddSystem<AsteroidFieldMovementSystem>("AsteroidFieldMovementSystem");

	// Textures
	std::vector<PBRTextureSet> textureSets;
	std::vector<TextureSetID> textureSetIDs;
	CreateAsteroidFieldTextures(MainTextureLoader, EngineRenderCore, textureSets, textureSetIDs);

	// Models
	auto rock = MainMeshLoader->GetMeshID("Rock.obj");
	auto rock2 = MainMeshLoader->GetMeshID("Rock2.obj");
	auto rock3 = MainMeshLoader->GetMeshID("Rock3.obj");
	auto rock4 = MainMeshLoader->GetMeshID("Rock4.obj");

	EngineRenderCore->RegisterMeshForRaytracingUsage(rock);
	EngineRenderCore->RegisterMeshForRaytracingUsage(rock2);
	EngineRenderCore->RegisterMeshForRaytracingUsage(rock3);
	EngineRenderCore->RegisterMeshForRaytracingUsage(rock4);

	std::vector<MeshID> meshIDs;
	meshIDs.push_back(rock);
	meshIDs.push_back(rock2);
	meshIDs.push_back(rock3);
	meshIDs.push_back(rock4);

	ModelMatrixComponent matrix = {};
	TransformComponent tc = {};
	AsteroidFieldMovementComponent movc = {};
	MaterialComponentRaytracing ray = {};


	float radiusMultiplier = 10.0f;

	for (size_t x = 0; x < asteroidCount; x++)
	{

		float rand1 = distr01(gen);
		float rand2 = distr01(gen);
		float rand3 = distr01(gen);
		float rand4 = distr01(gen);
		int textureSelect = std::round(distr01(gen) * (textureSets.size() - 1)) + 1;
		int meshSelect = std::round(distr01(gen) * (meshIDs.size() - 1));
		float speed = distr01(gen) * 7.5f + 2.0f;
		float textureRepeat = 1.0f + rand3 * 2.0f;
		MeshID usedMeshId = meshIDs[meshSelect];

		float scale = 0.05f + rand1 * 0.06f;
		tc = {
			.Position = glm::vec3(0,0,0),
			.Scale = glm::vec3(scale,scale,scale),
			.Orientation = glm::quat(glm::vec3(0,0,0)),
		};
		
		float radius = speed * radiusMultiplier + distr01(gen) * radiusMultiplier;
		movc.angleDirection = glm::vec2(0.01f * speed + rand1, 0.01f * speed + rand2);
		movc.goalAngle = glm::vec2(rand1 * 2.0f * glm::pi<float>(), rand2 * 2.0f * glm::pi<float>());
		movc.prevAngle = movc.goalAngle;
		movc.radius = radius;
		movc.goalPosition = CalculateGoalPosition(movc.radius, movc.goalAngle.x, movc.goalAngle.y);
		movc.speed = speed;

		tc.Position = movc.goalPosition;

		ray.TextureSet = textureSetIDs[textureSelect];
		ray.RenderMesh = usedMeshId;
		ray.TextureRepeat = textureRepeat;
		ray.Color = glm::vec3(1, 1, 1);


		ray.TextureSet = textureSelect;

		WorldEntityManager->CreateEntityWithData(tc, movc, matrix, ray);
	}
	tc.Position = glm::vec3(0, 0, 0);
}
