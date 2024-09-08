module;
#include <glm/glm.hpp>

export module EE_ECS_SystemData_InputSystem;

import <vector>;

export struct EE_InputSystemData
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
	bool Spacebar;
	bool Return;
	bool Left_Shift;
	bool Left_Control;
};
