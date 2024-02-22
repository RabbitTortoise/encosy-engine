module;
#include <SDL3/SDL.h>
#include <fmt/core.h>

export module EncosyEngine.Application;

import EncosyEngine.WindowManager;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;

import <iostream>;
import <chrono>;
import <thread>;


export class EncosyApplication
{
public:
    EncosyApplication() { }
    ~EncosyApplication() { }

    void EngineInit()
    {
        fmt::println("EncosyEngine Initialization");

        //Create new Window
        fmt::println("Window Creation");
        SDL_Init(SDL_INIT_VIDEO);
        EngineWindowManager = std::make_unique<WindowManager>();
        if (EngineWindowManager->CreateMainWindow())
        {
            MainWindow = EngineWindowManager->GetMainWindow();
        }

        // Setup EncosyCore
        fmt::println("EncosyCore Initialization");
        EngineEncosyCore = std::make_unique<EncosyCore>();
        PrimaryWorld = EngineEncosyCore->GetPrimaryWorld();

        // Setup RenderCore
        fmt::println("RenderCore Initialization");
        EngineRenderCore = std::make_unique<RenderCore>();
        EngineRenderCore->InitializeVulkan(MainWindow, PrimaryWorld);

        // Initialize Engine systems.
        EngineEncosyCore->InitCoreSystems(EngineWindowManager.get(), EngineRenderCore.get());

    }

    void EngineLoop()
    {

        using clock = std::chrono::high_resolution_clock;
        auto start = clock::now();

        while (!MainWindow->ShouldQuit())
        {
            auto end = clock::now();
            float deltaTime = std::chrono::duration<float>(end - start).count();

            // Here so that the game pauses when window is moved. Should be fixed when input handling is separated to own thread.
            // Poll other window events.
            MainWindow->PollEvents();

            start = clock::now();
            // Compute application frame time (delta time) and update application
           
            // Core PreUpdate
            // Game Update
            // Collision Update
            // System Update
            EngineEncosyCore->PrimaryWorldSystemUpdate(deltaTime);
            // Render Update
            if (EngineRenderCore->CheckIfRenderingConditionsMet())
            {
                EngineRenderCore->RenderStart();
                EngineEncosyCore->PrimaryWorldRenderUpdate(deltaTime);
                EngineRenderCore->EndRecording();
                EngineRenderCore->SubmitToQueue();
            }
            // Core LateUpdate
            //auto processEnd = clock::now();
            //auto processTime = (processEnd - start);
            //auto sleepfor = std::chrono::milliseconds(12) - processTime;
            //std::this_thread::sleep_for(sleepfor);
        }

        //Clean Engine Resources
        EngineRenderCore->WaitForGpuIdle();
        EngineRenderCore->Cleanup();
        fmt::println("Quitting Engine Loop");
    }

    WindowManager* GetWindowManager() { return EngineWindowManager.get(); }
    EncosyCore* GetEncosyCore() { return EngineEncosyCore.get(); }
    RenderCore* GetRenderCore() { return EngineRenderCore.get(); }

private:
    std::unique_ptr<WindowManager> EngineWindowManager;
    std::unique_ptr<RenderCore> EngineRenderCore;
    std::unique_ptr<EncosyCore> EngineEncosyCore;

    WindowInstance* MainWindow = nullptr;
    EncosyWorld* PrimaryWorld = nullptr;

};