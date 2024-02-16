module;
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <fmt/core.h>

export module EncosyEngine.WindowManager;

import <string>;
import <functional>;
import <vector>;

export class WindowInstance
{
    friend class WindowManager;
public:
    WindowInstance() {}
    WindowInstance(SDL_Window* window, int width, int height)
    {
        Window = window;
        WindowWidth = width;
        WindowHeight = height;
    }
    ~WindowInstance() {}

    void PollEvents()
    {
        bWasResized = false;
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (event.type == SDL_EVENT_QUIT)
            {
                bShouldQuit = true;
            }
            if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) 
            {
                switch (event.type) 
                {
                case SDL_EVENT_WINDOW_MINIMIZED:
                    bIsMinimized = true;
                    break;
                case SDL_EVENT_WINDOW_RESTORED:
                    bIsMinimized = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    bWasResized = true;
                    UpdateWindowDimensions();
                    break;
                }
            }
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
            if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    bShouldQuit = true;
                }
            }
            
            for (auto subscriber : EventSubscribers)
            {
                subscriber(event);
            }
        }
    }

    void SubscribeToEvents(std::function<void(SDL_Event e)> function)
    {
        EventSubscribers.push_back(function);
    }


    SDL_Window* GetWindow() { return Window; }
    const int GetWidth() const { return WindowWidth; }
    const int GetHeight() const { return WindowHeight; }
    const bool IsMinimized() const { return bIsMinimized; }
    const bool WasResized() const { return bWasResized; }
    const bool ShouldQuit() const { return bShouldQuit; }

private:

    void UpdateWindowDimensions()
    {
        SDL_GetWindowSize(Window, &WindowWidth, &WindowHeight);
        bool forceWindowSize = false;
        if (WindowWidth > 2560)
        {
            WindowWidth = 2560;
            forceWindowSize = true;
        }
        if (WindowHeight > 1440)
        {
            WindowHeight = 1440;
            forceWindowSize = true;
        }
        if (forceWindowSize)
        {
            SDL_SetWindowSize(Window, WindowWidth, WindowHeight);
        }
    }

    std::vector<std::function<void(SDL_Event e)>> EventSubscribers;
    SDL_Window* Window = nullptr;
    int WindowWidth = 1920;
    int WindowHeight = 1080;
    bool bShouldQuit = false;
    bool bIsMinimized = false;
    bool bWasResized = false;

};

export class WindowManager
{
public:
    WindowManager() {}
    ~WindowManager() { DestroyMainWindow(); }

    bool CreateMainWindow(std::string title = "EncosyEngine", int width = 1920, int height = 1080)
    {
       if (MainWindow.Window == nullptr)
        {
            MainWindow.Window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
            if (MainWindow.Window == nullptr)
            {
                fmt::println("ERROR: Window creation failed!");
                return false;
            }
            MainWindow.WindowWidth = width;
            MainWindow.WindowHeight = height;
            fmt::println("Window created");
            return true;
        }
        fmt::println("WARNING: Window already exists! Multiple windows are not supported");
        return false;
    }

    void DestroyMainWindow()
    {
        if (MainWindow.Window != nullptr)
        {
            SDL_DestroyWindow(MainWindow.Window);
            MainWindow.Window = nullptr;
        }
    }

    WindowInstance* GetMainWindow() { return &MainWindow; }

private:
    WindowInstance MainWindow;

};
