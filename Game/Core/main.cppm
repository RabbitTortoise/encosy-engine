module;
#include <fmt/core.h>
#include <glm/vec3.hpp>

export module EncosyGame;

import EncosyGame.RotationTest;
import EncosyGame.StaticTest;
import EncosyGame.CollisionDemo;
import EncosyGame.DynamicDemo;
import EncosyGame.Thesis;
import EncosyEngine.Interface;

enum class Scene { RotationTest = 0, StaticTest, CollisionDemo, DynamicDemo, Thesis};

Scene ChosenScene = Scene::Thesis;
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
		int testDimensionsX = 50;
		int testDimensionsY = 50;
		int testDimensionsZ = 50;

		InitRotationTest(testDimensionsX, testDimensionsY, testDimensionsZ);
	}
	if (ChosenScene == Scene::StaticTest)
	{
		int testDimensionsX = 20;
		int testDimensionsY = 20;
		int testDimensionsZ = 20;

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
	if (ChosenScene == Scene::Thesis)
	{
		int entityCount = 10000;
		int entitiesPerType = entityCount / 10;

		// Run options
		//InitThesisTestSingleThreaded(entitiesPerType);
		//InitThesisTestMultiThreaded(entitiesPerType, false);
		InitThesisTestMultiThreaded(entitiesPerType, true);
	}

	EncosyEngine::StartEngineLoop();
	fmt::println("Stopping EncosyGame process");
	return 0;
}


/* Used in thesis writing

import <vector>;
import <execution>;
import <algorithm>;
import <chrono>;
import <random>;
import <future>;


std::vector<int> globVec(50000);
	void ParallelSort()
	{
		std::sort(std::execution::par, std::begin(globVec), std::end(globVec));
	}

	void Sort()
	{
		std::ranges::sort(globVec);
	}

	// Nopeampi n. 50 000 numeron kohdalla

	//std::vector<int> globVec(50000);
	void AsyncTest()
	{
		auto find_max = [](std::vector<int>&& vec) -> int
			{ return (*std::ranges::max_element(vec)); };
		auto f = std::async(std::launch::async, find_max, globVec);
		int result = f.get();
	}


	//using namespace std::chrono;
	//std::mt19937 rng;
	//rng.seed(0);
	//std::uniform_int_distribution<int> dist(-1000000000, 1000000000);
	//auto gen = [&dist, &rng]() { return dist(rng); };
	//std::generate(std::begin(globVec), std::end(globVec), gen);
	//auto frameStart = steady_clock::now();
	////ParallelSort();
	////Sort();
	//AsyncTest();
	//auto frameEnd = steady_clock::now();
	//double frameTime = std::chrono::duration<double>(frameEnd - frameStart).count();
	//fmt::println("Result: {}", frameTime);
	//return 0;

*/