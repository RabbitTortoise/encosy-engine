module;

export module EncosyEngine.Interface;

import EncosyEngine.Application;
import EncosyEngine.WindowManager;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;


import <vector>;
import <string>;
import <random>;
import <iostream>;
import <memory>;


namespace
{
    std::unique_ptr<EncosyApplication> EngineApp;
}

export namespace EncosyEngine
{
    void InitializeEngine(std::string title = "EncosyEngine", bool fullscreen = false, int width = 1920, int height = 1080)
    {
        // Initialise Random number generator
        std::random_device rd;
        std::mt19937 random_mt(rd());

        std::cout << "Starting EncosyEngine" << std::endl;
        EngineApp = std::make_unique<EncosyApplication>();
        EngineApp->EngineInit(title, fullscreen, width, height);
    }

    void StartEngineLoop()
    {
        EngineApp->EngineLoop();
    }

    WindowManager* GetWindowManager() { return EngineApp->GetWindowManager(); }
    EncosyCore* GetEncosyCore() { return EngineApp->GetEncosyCore(); }
    RenderCore* GetRenderCore() { return EngineApp->GetRenderCore(); }
}
