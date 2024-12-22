module;
#include <fmt/core.h>
#include <glm/vec3.hpp>

export module EncosyGame;

import EncosyEngine_EngineCore;

import EncosyGame_RotationTest;
import EncosyGame_StaticTest;
import EncosyGame_CollisionDemo;
import EncosyGame_DynamicDemo;
import EncosyGame_RaytracingTest;
import EncosyGame_AsteroidField;

enum class Scene { RotationTest = 0, StaticTest, CollisionDemo, DynamicDemo, Raytracing, AsteroidField};

Scene ChosenScene = Scene::StaticTest;
bool Fullscreen = false;

export
int main()
{
	fmt::println("Started EncosyGame process");
	if (Fullscreen)
	{
		EncosyEngine::InitializeEngine("EncosyEngine", true, 2560, 1440);
	}
	else
	{
		EncosyEngine::InitializeEngine("EncosyEngine", false, 1920, 1080);
	}

	if(ChosenScene == Scene::RotationTest)
	{
		int testDimensionsX = 10;
		int testDimensionsY = 10;
		int testDimensionsZ = 10;

		InitRotationTest(testDimensionsX, testDimensionsY, testDimensionsZ);
	}
	if (ChosenScene == Scene::StaticTest)
	{
		int testDimensionsX = 30;
		int testDimensionsY = 30;
		int testDimensionsZ = 30;

		InitStaticTest(testDimensionsX, testDimensionsY, testDimensionsZ);
	}
	if (ChosenScene == Scene::CollisionDemo)
	{
		glm::vec3 playRegionMin = { -100, -70, -80 };
		glm::vec3 playnRegionMax = { 100, 70, -50 };
		InitCollisionDemo(playRegionMin, playnRegionMax);
	}
	if (ChosenScene == Scene::DynamicDemo)
	{
		glm::vec3 playRegionMin = { -100, -70, -70 };
		glm::vec3 playnRegionMax = { 100, 70, -50 };
		InitDynamicDemo(playRegionMin, playnRegionMax);
	}

	if (ChosenScene == Scene::Raytracing)
	{
		int testDimensionsX = 20;
		int testDimensionsY = 20;
		int testDimensionsZ = 20;

		InitRaytracingTest(testDimensionsX, testDimensionsY, testDimensionsZ);
	}
	if (ChosenScene == Scene::AsteroidField)
	{
		unsigned int asteroidCount = 250000;
		InitAsteroidField(asteroidCount);
	}

	EncosyEngine::StartEngineLoop();
	fmt::println("Stopping EncosyGame process");
	return 0;
}
