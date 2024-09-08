module;
#include <memory>
#include <SDL3/SDL.h>
#include <fmt/core.h>

export module EE_Core_WindowManager;

import EE_Core_WM_WindowInstance;

import <string>;
import <functional>;
import <vector>;


namespace
{
    std::unique_ptr<WindowInstance> MainWindow;
}

export namespace EncosyEngine::WindowManager
{
    void InitWindowManager()
    {
        MainWindow = std::make_unique<WindowInstance>();
    }

    bool CreateMainWindow(std::string title = "EncosyEngine", bool fullscreen = false, int width = 1920, int height = 1080)
    {
        if (MainWindow->GetWindow() == nullptr)
        {
            if (fullscreen)
            {
                MainWindow->SetWindow(SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN));
            }
            else
            {
                MainWindow->SetWindow(SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE));
            }
            if (MainWindow->GetWindow() == nullptr)
            {
                fmt::println("ERROR: Window creation failed!");
                return false;
            }
            MainWindow->SetWidth(width);
            MainWindow->SetHeight(height);
            return true;
        }
        fmt::println("WARNING: Main window already exists! Multiple windows are not currently supported");
        return false;
    }

    void DestroyMainWindow()
    {
        if (MainWindow->GetWindow() != nullptr)
        {
            MainWindow->DestroyWindowInstance();
        }
    }

    void PollMainWindowEvents()
    {
        MainWindow->PollEvents();
    }

    void SubscribeToMainWindowEvents(std::function<void(SDL_Event e)> function)
    {
        MainWindow->SubscribeToEvents(function);
    }

    void SetRelativeMouseModeForMainWindow(bool mode)
    {
        MainWindow->SetRelativeMouseMode(mode);
    }

    SDL_Window* GetMainWindow()
    {
        return MainWindow->GetWindow();
    }

    int GetMainWindowWidth() { return MainWindow->GetWidth(); }
    int GetMainWindowHeight() { return MainWindow->GetHeight(); }
    bool IsMainWindowMinimized() { return MainWindow->IsMinimized(); }
    bool WasMainWindowResized() { return MainWindow->WasResized(); }
    bool ShouldMainWindowQuit() { return MainWindow->ShouldQuit(); }

}
