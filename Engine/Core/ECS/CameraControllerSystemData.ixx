module;
#include <glm/glm.hpp>

export module SystemData.CameraControllerSystem;

import ECS.Entity;
import <vector>;

export struct CameraControllerSystemData
{
	Entity MainCamera;

	//Debug Camera Movement
	float Yaw = -90;
	float Pitch = 0;
};
