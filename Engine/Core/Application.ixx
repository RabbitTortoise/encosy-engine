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

    void EngineInit(std::string title, bool fullscreen, int width, int height)
    {
        fmt::println("EncosyEngine Initialization");

        //Create new Window
        fmt::println("Window Creation");
        SDL_Init(SDL_INIT_VIDEO);
        EngineWindowManager = std::make_unique<WindowManager>();
        if (EngineWindowManager->CreateMainWindow(title, fullscreen, width, height))
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
        using clock = std::chrono::steady_clock;
        auto frameStart = clock::now();
        auto frameEnd = frameStart;

        auto fixedPhysicsDeltaTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(1.0 / MaxPhysicsFPS));
        auto accumulatedPhysicsTime = std::chrono::nanoseconds(0);
        auto simulatedPhysicsTime = std::chrono::nanoseconds(0);

        auto fixedUpdateDeltaTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(1.0 / MaxUpdateFPS));
        auto accumulatedUpdateTime = std::chrono::nanoseconds(0);
        auto simulatedUpdateTime = std::chrono::nanoseconds(0);

        auto fixedRenderDeltaTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(1.0 / MaxRenderFPS));
        auto accumulatedRenderTime = std::chrono::nanoseconds(0);

        LastFrametimes = std::vector<std::chrono::nanoseconds>(LastFramesCount, fixedRenderDeltaTime);

        while (!MainWindow->ShouldQuit())
        {
            // Compute application frame time (delta time) and update application
            frameEnd = clock::now();
            const auto frameTime = frameEnd - frameStart;
            frameStart = frameEnd;

            accumulatedPhysicsTime += std::chrono::nanoseconds(frameTime);
            accumulatedUpdateTime += std::chrono::nanoseconds(frameTime);
            accumulatedRenderTime += std::chrono::nanoseconds(frameTime);

            // Calculate average of last frames
            LastFrametimes[CurrentFrameTimeIndex] = frameTime;
            CurrentFrameTimeIndex++;
            if (CurrentFrameTimeIndex == LastFramesCount) { CurrentFrameTimeIndex = 0; }
            auto lastFrameTimes = std::accumulate(LastFrametimes.begin(), LastFrametimes.end(), LastFrametimes[0], [](const auto a, const auto b)
                {
                    return a + b;
                });
            auto averageFrameTime = lastFrameTimes / LastFramesCount;


            // Poll other window events.
            MainWindow->PollEvents();


            // Physics System Update
            auto physicsDeltaTime = fixedPhysicsDeltaTime;
            simulatedPhysicsTime = std::chrono::nanoseconds(0);
            while (accumulatedPhysicsTime >= physicsDeltaTime)
            {
                // Run the simulation with less accuracy if the fps is low.
                while(accumulatedPhysicsTime > physicsDeltaTime * 2.0) { physicsDeltaTime *= 2.0; }
             
                accumulatedPhysicsTime -= physicsDeltaTime;
                simulatedPhysicsTime += physicsDeltaTime;

                const double deltaTime = std::chrono::duration<double>(physicsDeltaTime).count();

                EngineEncosyCore->PrimaryWorldPhysicsUpdate(deltaTime);
            }

            // System Update
            auto updateDeltaTime = fixedUpdateDeltaTime;
            simulatedUpdateTime = std::chrono::nanoseconds(0);
            if (accumulatedUpdateTime > updateDeltaTime)
            {
                // Unbound render fps if much below set target
                while (accumulatedUpdateTime > updateDeltaTime * 2.0) { updateDeltaTime *= 2.0; }

                if (accumulatedUpdateTime > updateDeltaTime * 2.0) { updateDeltaTime = accumulatedUpdateTime; }
                accumulatedUpdateTime -= updateDeltaTime;
                simulatedUpdateTime += physicsDeltaTime;

                const double deltaTime = std::chrono::duration<double>(updateDeltaTime).count();

                EngineEncosyCore->PrimaryWorldSystemUpdate(deltaTime);
            }

            // Render System Update
            if (EngineRenderCore->CheckIfRenderingConditionsMet())
            {
                const double deltaTime = std::chrono::duration<double>(frameTime).count();
                EngineRenderCore->RenderStart();
                EngineEncosyCore->PrimaryWorldRenderUpdate(deltaTime);
                EngineRenderCore->EndRecording();
                EngineRenderCore->SubmitToQueue();
            }

            // Framerate Cap
            auto desired_end = frameStart + fixedRenderDeltaTime - (averageFrameTime - fixedRenderDeltaTime);
            std::this_thread::sleep_until(desired_end);
        }

        // Clean Engine Resources
        EngineRenderCore->WaitForGpuIdle();
        EngineRenderCore->Cleanup();
        fmt::println("Quitting Engine Loop");
    }

    WindowManager* GetWindowManager() { return EngineWindowManager.get(); }
    EncosyCore* GetEncosyCore() { return EngineEncosyCore.get(); }
    RenderCore* GetRenderCore() { return EngineRenderCore.get(); }

private:
    // Update framerates
    double MaxPhysicsFPS = 60.0;
    double MaxUpdateFPS = 120.0;
    double MaxRenderFPS = 240.0;

    // Average FPS Calculation
    int LastFramesCount = 3;
    std::vector<std::chrono::nanoseconds> LastFrametimes;
    int CurrentFrameTimeIndex = 0;

    std::unique_ptr<WindowManager> EngineWindowManager;
    std::unique_ptr<RenderCore> EngineRenderCore;
    std::unique_ptr<EncosyCore> EngineEncosyCore;

    WindowInstance* MainWindow = nullptr;
    EncosyWorld* PrimaryWorld = nullptr;
};