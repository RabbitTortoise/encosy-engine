module;
#include <fmt/core.h>

export module EncosyGame;

import EncosyGame.StressTest;
import EncosyEngine.Interface;


enum class Scene { StressTest = 0, Demo1};

Scene ChosenScene = Scene::StressTest;

export
int main()
{
	fmt::println("Started EncosyGame process");
	EncosyEngine::InitializeEngine();


	if(ChosenScene == Scene::StressTest)
	{
		int testDimensionsX = 20;
		int testDimensionsY = 20;
		int testDimensionsZ = 20;

		InitStressTest(testDimensionsX, testDimensionsY, testDimensionsZ);
	}
	if (ChosenScene == Scene::Demo1)
	{

	}


	EncosyEngine::StartEngineLoop();
	fmt::println("Stopping EncosyGame process");
	return 0;
}
