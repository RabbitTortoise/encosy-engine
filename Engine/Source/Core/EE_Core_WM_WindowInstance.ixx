module;
#include <SDL3/SDL.h>


export module EE_Core_WM_WindowInstance;

import <string>;
import <functional>;
import <vector>;

export class WindowInstance
{
public:
    WindowInstance() {}
    ~WindowInstance() { DestroyWindowInstance(); }

    void DestroyWindowInstance()
    {
        if (Window != nullptr)
        {
            SDL_DestroyWindow(Window);
            Window = nullptr;
        }
    }

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

    void SetRelativeMouseMode(bool mode)
    {
        SDL_SetRelativeMouseMode(mode);
    }

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

    void SetWindow(SDL_Window* newWindow)
    {
	    if(Window != nullptr)
	    {
            SDL_DestroyWindow(Window);
            Window = nullptr;
	    }
        Window = newWindow;
    }
    void SetWidth(int newWidth) { WindowWidth = newWidth; }
    void SetHeight(int newHeight) { WindowHeight = newHeight; }


    SDL_Window* GetWindow() { return Window; }
    int GetWidth() const { return WindowWidth; }
    int GetHeight() const { return WindowHeight; }
    bool IsMinimized() const { return bIsMinimized; }
    bool WasResized() const { return bWasResized; }
    bool ShouldQuit() const { return bShouldQuit; }

private:

    SDL_Window* Window = nullptr;

    std::vector<std::function<void(SDL_Event e)>> EventSubscribers;
    int WindowWidth = 1920;
    int WindowHeight = 1080;
    bool bShouldQuit = false;
    bool bIsMinimized = false;
    bool bWasResized = false;

};