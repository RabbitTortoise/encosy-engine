module;

export module EncosyEngine_EngineCore;

import EE_Core_EngineCore;
import EE_RenderCore;


import <vector>;
import <string>;
import <random>;
import <iostream>;
import <memory>;


namespace
{
    std::unique_ptr<EngineCore> EngineCoreObject;
}

export namespace EncosyEngine
{
    void InitializeEngine(std::string title = "EncosyEngine", bool fullscreen = false, int width = 1920, int height = 1080)
    {
	    std::cout << "Starting EncosyEngine" << std::endl;
        EngineCoreObject = std::make_unique<EngineCore>();
        EngineCoreObject->EngineInit(title, fullscreen, width, height);
    }

    void StartEngineLoop()
    {
        EngineCoreObject->EngineLoop();
    }

    EncosyCore* GetEncosyCore() { return EngineCoreObject->GetEncosyCore(); }
    RenderCore* GetRenderCore() { return EngineCoreObject->GetRenderCore(); }
}
