module;
#include <sdl3/SDL.h>
#include <glm/glm.hpp>

export module SystemData.InputSystem;

import <vector>;

export struct InputSystemData
{
	glm::vec2 MousePosition;
	glm::vec2 MouseRelativeMotion;
	bool MouseRightDown;
	bool MouseLeftDown;
	bool MouseLeftClicked;
	bool MouseRightClicked;
	bool A;
	bool S;
	bool D;
	bool W;
	bool Q;
	bool E;
	bool Left_Shift;
	bool Left_Control;
};
