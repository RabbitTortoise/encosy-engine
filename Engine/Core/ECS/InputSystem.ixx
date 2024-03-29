module;
#include <SDL3/SDL.h>
#include <fmt/core.h>

export module Systems.InputSystem;

import EncosyCore.System;
import SystemData.InputSystem;
import EncosyEngine.WindowManager;

import <map>;
import <span>;
import <vector>;
import <iostream>;
import <queue>;


export
class InputSystem : public System
{
	friend class SystemManager;

public:
	InputSystem() {}
	~InputSystem() {}

	void AddEventToQueue(SDL_Event e)
	{
		EventsInQueue.push(e);
	}

protected:
	void Init() override 
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::First;

		AddSystemDataForWriting(&InputSystemDataComponent);
	}
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override
	{
		UpdateInputStates(deltaTime);
	}
	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override {}
	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}


	void UpdateInputStates(const double deltaTime)
	{
		auto& inputData = GetSystemData(&InputSystemDataComponent);

		inputData.MouseLeftClicked = false;
		inputData.MouseRightClicked = false;
		inputData.MouseRelativeMotion.x = 0;
		inputData.MouseRelativeMotion.y = 0;

		while (!EventsInQueue.empty())
		{
			// Key press event
			SDL_Event event = EventsInQueue.front();
			EventsInQueue.pop();
			bool keyDown = (event.type == SDL_EVENT_KEY_DOWN);
			bool keyUp = (event.type == SDL_EVENT_KEY_UP);
			if (keyDown || keyUp)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_w:
					inputData.W = keyDown;
					break;
				case SDLK_a:
					inputData.A = keyDown;
					break;
				case SDLK_s:
					inputData.S = keyDown;
					break;
				case SDLK_d:
					inputData.D = keyDown;
					break;
				case SDLK_q:
					inputData.Q = keyDown;
					break;
				case SDLK_e:
					inputData.E = keyDown;
					break;
				case SDLK_SPACE:
					inputData.Spacebar = keyDown;
					break;
				case SDLK_RETURN:
					inputData.Return = keyDown;
					break;
				case SDLK_LSHIFT:
					inputData.Left_Shift = keyDown;
					break;
				case SDLK_LCTRL:
					inputData.Left_Control = keyDown;
					break;
				}
				continue;
			}

			// Mouse button event
			bool mouseDown = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
			bool mouseUp = (event.type == SDL_EVENT_MOUSE_BUTTON_UP);
			if (mouseDown || mouseUp)
			{
				switch (event.button.button)
				{
				case 1:
					if (event.button.state == SDL_PRESSED)
					{
						inputData.MouseLeftClicked = true;
						inputData.MouseLeftDown = true;
					}
					else
					{
						inputData.MouseLeftDown = false;
					}
					break;
				
				case 3:
					if (event.button.state == SDL_PRESSED)
					{
						inputData.MouseRightClicked = true;
						inputData.MouseRightDown = true;
					}
					else
					{
						inputData.MouseRightDown = false;
					}
					break;
				}
				continue;
			}

			// Mouse motion event
			bool mouseMotion = (event.type == SDL_EVENT_MOUSE_MOTION);
			if (mouseMotion)
			{
				inputData.MousePosition.x = event.motion.x;
				inputData.MousePosition.y = event.motion.y;
				inputData.MouseRelativeMotion.x = event.motion.xrel;
				inputData.MouseRelativeMotion.y = event.motion.yrel;
				continue;
			}
		}
	}

private:
	std::queue<SDL_Event> EventsInQueue;

	WriteReadSystemDataStorage<InputSystemData> InputSystemDataComponent;

};
