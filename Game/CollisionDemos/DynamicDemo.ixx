module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


export module EncosyGame.DynamicDemo;

import EncosyEngine.Interface;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;

import Components.TransformComponent;
import Components.MaterialComponent;
import Components.ModelMatrixComponent;
import Demo.Components.LeaderComponent;
import Demo.Components.FollowerComponent;
import Demo.Components.MovementComponent;
import Demo.Components.CollisionEventComponent;
import Demo.Components.SphereColliderComponent;
import Demo.Components.DyingFollowerComponent;
import Demo.Systems.SpawningSystem;
import Demo.Systems.DyingFollowerSystem;
import Demo.Systems.FollowerMovementSystem;
import Demo.Systems.LeaderMovementSystem;
import Demo.Systems.SphereCollisionSystem;
import Demo.Systems.FollowerModifySystem;
import Demo.Systems.FollowerKillingSystem;
import Demo.SystemData.SpawningSystem;

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


export void InitDynamicDemo(glm::vec3 playRegionMin, glm::vec3 playRegionMax)
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

	auto EngineCore = EncosyEngine::GetEncosyCore();
	auto PrimaryWorld = EngineCore->GetPrimaryWorld();

	auto WorldComponentManager = PrimaryWorld->GetWorldComponentManager();
	auto WorldEntityManager = PrimaryWorld->GetWorldEntityManager();
	auto WorldSystemManager = PrimaryWorld->GetWorldSystemManager();

	auto EngineRenderCore = EncosyEngine::GetRenderCore();
	auto MainTextureLoader = EngineRenderCore->GetTextureLoader();
	auto MainMeshLoader = EngineRenderCore->GetMeshLoader();
	auto MainRenderPipelineManager = EngineRenderCore->GetRenderPipelineManager();

	auto leaderInfo = WorldEntityManager->CreateEntityType<
		TransformComponent,
		MaterialComponentRaytracing,
		LeaderComponent,
		MovementComponent>
		("LeaderEntity");
	auto followerInfo = WorldEntityManager->CreateEntityType<
		TransformComponent,
		MaterialComponentRaytracing,
		ModelMatrixComponent,
		FollowerComponent,
		MovementComponent,
		SphereColliderComponent>
		("FollowerEntity");
	auto dyingFollowerInfo = WorldEntityManager->CreateEntityType<
		TransformComponent,
		MaterialComponentRaytracing,
		ModelMatrixComponent,
		FollowerComponent,
		DyingFollowerComponent,
		SphereColliderComponent>
		("DyingFollowerEntity");


	WorldComponentManager->CreateComponentStorage<CollisionEventComponent>();

	glm::vec3 Position;
	glm::vec3 Scale;
	glm::quat Orientation;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distrX(playRegionMin.x, playRegionMax.x);
	std::uniform_real_distribution<float> distrY(playRegionMin.y, playRegionMax.y);
	std::uniform_real_distribution<float> distrZ(playRegionMin.z, playRegionMax.z);

	// Textures
	std::vector<PBRTextureSet> textureSets;
	std::vector<TextureSetID> textureSetIDs;
	CreateTextures(MainTextureLoader, EngineRenderCore, textureSets, textureSetIDs);

	auto torus = MainMeshLoader->GetEngineMeshID(EngineMesh::Torus);
	auto IcoMeshID = MainMeshLoader->GetMeshID("DeadFollower.obj");
	EngineRenderCore->RegisterMeshForRaytracingUsage(IcoMeshID);

	MovementComponent mov = { {}, {8.0f} };
	SpawningSystemData ssData;

	for (size_t i = 0; i < 5; i++)
	{
		MaterialComponentRaytracing mat1 = { 1, torus, 1.0f, glm::vec3(1,1,1)};
		MaterialComponentRaytracing mat2 = { 2, torus, 1.0f, glm::vec3(1,1,1)};
		MaterialComponentRaytracing mat3 = { 3, torus, 1.0f, glm::vec3(1,1,1)};
		MaterialComponentRaytracing mat4 = { 4, torus, 1.0f, glm::vec3(1,1,1)};
		MaterialComponentRaytracing mat5 = { 5, torus, 1.0f, glm::vec3(1,1,1)};
		LeaderComponent lc1 = { 1, {distrX(gen),distrY(gen),distrZ(gen) } };
		LeaderComponent lc2 = { 2, {distrX(gen),distrY(gen),distrZ(gen) } };
		LeaderComponent lc3 = { 3, {distrX(gen),distrY(gen),distrZ(gen) } };
		LeaderComponent lc4 = { 4, {distrX(gen),distrY(gen),distrZ(gen) } };
		LeaderComponent lc5 = { 5, {distrX(gen),distrY(gen),distrZ(gen) } };

		TransformComponent tc = { {distrX(gen),distrY(gen),distrZ(gen) },  {0,0,0},  glm::quat() };
		auto createdEntity1 = WorldEntityManager->CreateEntityWithData(leaderInfo.Type, tc, mat1, lc1, mov);

		tc = { {distrX(gen),distrY(gen),distrZ(gen) },  {0,0,0},  glm::quat() };
		auto createdEntity2 = WorldEntityManager->CreateEntityWithData(leaderInfo.Type, tc, mat2, lc2, mov);

		tc = { {distrX(gen),distrY(gen),distrZ(gen) },  {0,0,0},  glm::quat() };
		auto createdEntity3 = WorldEntityManager->CreateEntityWithData(leaderInfo.Type, tc, mat3, lc3, mov);

		tc = { {distrX(gen),distrY(gen),distrZ(gen) },  {0,0,0},  glm::quat() };
		auto createdEntity4 = WorldEntityManager->CreateEntityWithData(leaderInfo.Type, tc, mat4, lc4, mov);

		tc = { {distrX(gen),distrY(gen),distrZ(gen) },  {0,0,0},  glm::quat() };
		auto createdEntity5 = WorldEntityManager->CreateEntityWithData(leaderInfo.Type, tc, mat5, lc5, mov);

		ssData.CurrentLeaders.push_back(createdEntity1);
		ssData.CurrentLeaders.push_back(createdEntity2);
		ssData.CurrentLeaders.push_back(createdEntity3);
		ssData.CurrentLeaders.push_back(createdEntity4);
		ssData.CurrentLeaders.push_back(createdEntity5);

		ssData.CurrentLeaderIndexes.push_back(lc1.LeaderID);
		ssData.CurrentLeaderIndexes.push_back(lc2.LeaderID);
		ssData.CurrentLeaderIndexes.push_back(lc3.LeaderID);
		ssData.CurrentLeaderIndexes.push_back(lc4.LeaderID);
		ssData.CurrentLeaderIndexes.push_back(lc5.LeaderID);

		ssData.LeaderMaterial.push_back(mat1);
		ssData.LeaderMaterial.push_back(mat2);
		ssData.LeaderMaterial.push_back(mat3);
		ssData.LeaderMaterial.push_back(mat4);
		ssData.LeaderMaterial.push_back(mat5);
	}

	ssData.PlayRegionMin = playRegionMin;
	ssData.PlayRegionMax = playRegionMax;

	ssData.minFPS = 60;
	ssData.resetFPS = 75;
	ssData.textureSets = textureSets;

	WorldComponentManager->CreateSystemDataStorage(ssData);


	WorldSystemManager->AddSystem<SpawningSystem>("DemoSpawningSystem", MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<LeaderMovementSystem>("DemoLeaderMovementSystem");
	WorldSystemManager->AddSystem<SphereCollisionSystem>("DemoSphereCollisionSystem");
	WorldSystemManager->AddSystem<FollowerModifySystem>("DemoFollowerModifySystem", MainMeshLoader);
	WorldSystemManager->AddSystem<FollowerKillingSystem>("DemoFollowerKillingSystem");
	WorldSystemManager->AddSystem<FollowerMovementSystem>("DemoFollowerMovementSystem");
	WorldSystemManager->AddSystem<DyingFollowerSystem>("DemoDyingFollowerSystem");
}