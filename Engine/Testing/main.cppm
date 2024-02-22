module;

#include <locale.h>
#include <Windows.h>
#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EncosyTesting;

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



void Tests()
{
	auto EngineCore = EncosyEngine::GetEncosyCore();
	auto PrimaryWorld = EngineCore->GetPrimaryWorld();

	auto WorldComponentManager = PrimaryWorld->GetWorldComponentManager();
	auto WorldEntityManager = PrimaryWorld->GetWorldEntityManager();
	auto WorldSystemManager = PrimaryWorld->GetWorldSystemManager();

}

void TestThreadedTaskRunner()
{
	ThreadedTaskRunner ThreadingTest;

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distributionInt(0, 2);
	std::uniform_real_distribution<float> distributionFloat(.0f, .9f);
	std::uniform_real_distribution<double> distributionDouble(.0, .9);

	//Add Random Tasks
	int threads = ThreadingTest.GetThreadCount() / 2;
	std::vector<double> results;
	for (size_t i = 0; i < threads; i++)
	{
		results.push_back(0);
	}

	for (size_t i = 0; i < threads; ++i)
	{
		int inx = distributionInt(generator);
		float iny = distributionFloat(generator);
		double inz = distributionDouble(generator);

		ThreadingTest.AddTask([=](int x, float y, double z, double* result) ->void {
			double sleepDur = z + y + x;
			for (size_t ix = 0; ix < 10000; ++ix)
			{
				sleepDur += sleepDur / 10000;
			}
			fmt::println("Result: {}; {}; {} = {}", x, y, z, sleepDur);
			*result = sleepDur;

			}, inx, iny, inz, &results[i]);
	}

	ThreadingTest.RunAllTasks();
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


	TransformComponent tc = {
	.Position = glm::vec3(0,-2,0),
	.Scale = glm::vec3(10,10,1),
	.Orientation = glm::quat(glm::vec3(glm::radians(90.0f),0,0)),
	};
	MaterialComponentUnlit mc = {
		.Diffuse = error,
		.RenderMesh = MainMeshLoader->GetEngineMeshID(EngineMesh::Quad),
		.TextureRepeat = 2.0f
	};
	WorldEntityManager->CreateEntityWithData(tc, mc);

	mc = {
		.Diffuse = error,
		.RenderMesh = MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere),
		.TextureRepeat = 1.0f
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
    std::cout << "Started EncosyEngine test process" << std::endl;
    
	EncosyEngine::InitializeEngine();

	TestThreadedTaskRunner();
	//Tests();
	InitializeTestEntities();

	EncosyEngine::StartEngineLoop();
	
    std::cout << "Stopping EncosyEngine test process" << std::endl;
    return 0;
}
