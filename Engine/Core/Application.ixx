module;
#include <chrono>
#include <SDL3/SDL.h>
#include <fmt/core.h>

export module EncosyEngine.Application;

import EncosyEngine.WindowManager;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;

import <thread>;
import <numeric>;
import <vector>;
import <ratio>;
import <string>;


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

        using namespace std::chrono;

        auto frameStart = steady_clock::now();
        auto frameEnd = frameStart;

        double fixedPhysicsDeltaTime = 1.0 / MaxPhysicsFPS;
        double physicsAccuracyTreshold = fixedPhysicsDeltaTime * 2.0;
        double accumulatedPhysicsTime = 0.0;
        double simulatedPhysicsTime = 0.0;

        double fixedUpdateDeltaTime = 1.0 / MaxUpdateFPS;
        double updateAccuracyTreshold = fixedUpdateDeltaTime * 2.0;
        double accumulatedUpdateTime = 0.0;
        double simulatedUpdateTime = 0.0;

        double fixedRenderDeltaTime = 1.0 / MaxRenderFPS;
        auto fixedRenderDeltaTimeNano = nanoseconds(static_cast<int>(floor(fixedRenderDeltaTime * 1000000000)));
        double accumulatedRenderTime = 0.0;

        LastFrametimes = std::vector<double>(LastFramesCount, fixedRenderDeltaTime);

        while (!MainWindow->ShouldQuit())
        {
            // Compute application frame time (delta time) and update application
            frameEnd = steady_clock::now();
            const auto clockFrameTime = frameEnd - frameStart;
            double frameTime = std::chrono::duration<double>(clockFrameTime).count();
            frameStart = frameEnd;

            accumulatedPhysicsTime += frameTime;
            accumulatedUpdateTime += frameTime;
            accumulatedRenderTime += frameTime;

            // Calculate average of last frames
            LastFrametimes[CurrentFrameTimeIndex] = frameTime;
            CurrentFrameTimeIndex++;
            if (CurrentFrameTimeIndex == LastFramesCount) { CurrentFrameTimeIndex = 0; }
            double averageFrameTime = std::accumulate(LastFrametimes.begin(), LastFrametimes.end(), 0.0) / LastFrametimes.size();

            // Poll other window events.
            MainWindow->PollEvents();
            // Update System manager state.
            EngineEncosyCore->PrimaryWorldSystemManagerUpdate();

            // Physics System Update
            auto physicsDeltaTime = fixedPhysicsDeltaTime;
            simulatedPhysicsTime = 0;
            // Run the simulation with less accuracy if the fps is low.
            if (accumulatedPhysicsTime > physicsAccuracyTreshold)
            {
                physicsDeltaTime = accumulatedPhysicsTime;
            }
            if (accumulatedPhysicsTime >= physicsDeltaTime)
            {
                accumulatedPhysicsTime -= physicsDeltaTime;
                simulatedPhysicsTime += physicsDeltaTime;

                EngineEncosyCore->PrimaryWorldPhysicsUpdate(physicsDeltaTime);
            }

            // System Update
            auto updateDeltaTime = fixedUpdateDeltaTime;
            simulatedUpdateTime = 0;
            // Run the simulation with less accuracy if the fps is low.
            if (accumulatedUpdateTime > updateAccuracyTreshold)
            {
                updateDeltaTime = accumulatedUpdateTime;
            }
            if (accumulatedUpdateTime >= updateDeltaTime)
            {
                accumulatedUpdateTime -= updateDeltaTime;
                simulatedUpdateTime += physicsDeltaTime;

                EngineEncosyCore->PrimaryWorldSystemUpdate(updateDeltaTime);
            }

            // Render System Update
            if (EngineRenderCore->CheckIfRenderingConditionsMet())
            {
                EngineRenderCore->RenderStart();
                EngineEncosyCore->PrimaryWorldRenderUpdate(frameTime);
                EngineRenderCore->EndRecording();
                EngineRenderCore->SubmitToQueue();
            }

            // Framerate Cap
            double endOffset = fixedRenderDeltaTime - (averageFrameTime - fixedRenderDeltaTime);
            nanoseconds offsetNanoseconds = nanoseconds(static_cast<int>(floor(endOffset * 1000000000)));
            auto desired_end = frameStart + offsetNanoseconds;
            sleep_until_busy(desired_end);
        }

        // Clean Engine Resources
        EngineRenderCore->WaitForGpuIdle();
        EngineRenderCore->Cleanup();
        EngineEncosyCore->Cleanup();
      
        fmt::println("Quitting Engine Loop");
    }

    template <class Clock, class Duration>
    void sleep_until_busy(std::chrono::time_point<Clock, Duration> tp)
    {
        using namespace std::chrono;
        auto sleepTp = tp - microseconds(1000);
        std::this_thread::sleep_until(sleepTp);
        while (tp >= Clock::now())
        {
            ;
        }
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
    std::vector<double> LastFrametimes;
    int CurrentFrameTimeIndex = 0;

    std::unique_ptr<WindowManager> EngineWindowManager;
    std::unique_ptr<RenderCore> EngineRenderCore;
    std::unique_ptr<EncosyCore> EngineEncosyCore;

    WindowInstance* MainWindow = nullptr;
    EncosyWorld* PrimaryWorld = nullptr;
};