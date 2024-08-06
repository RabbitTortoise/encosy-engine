module;
#include <glm/glm.hpp>

export module SystemData.CameraControllerSystem;

import EncosyCore.Entity;
import EncosyEngine.WindowManager;

import <vector>;

export struct CameraControllerSystemData
{
	Entity MainCamera;
	WindowInstance* MainWindow;

	//Debug Camera Movement
	float CurrentYaw = -90;
	float CurrentPitch = 0;

	float DesiredYaw = -90;
	float DesiredPitch = 0;
};
