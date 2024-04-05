module;
#include <fmt/core.h>
#include <glm/vec3.hpp>

export module EncosyGame;

import EncosyGame.RotationTest;
import EncosyGame.StaticTest;
import EncosyGame.CollisionDemo;
import EncosyGame.DynamicDemo;
import EncosyEngine.Interface;


enum class Scene { RotationTest = 0, StaticTest, CollisionDemo, DynamicDemo};

Scene ChosenScene = Scene::DynamicDemo;
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
		int testDimensionsX = 25;
		int testDimensionsY = 25;
		int testDimensionsZ = 25;

		InitRotationTest(testDimensionsX, testDimensionsY, testDimensionsZ);
	}
	if (ChosenScene == Scene::StaticTest)
	{
		int testDimensionsX = 25;
		int testDimensionsY = 25;
		int testDimensionsZ = 25;

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
	
	EncosyEngine::StartEngineLoop();
	fmt::println("Stopping EncosyGame process");
	return 0;
}
