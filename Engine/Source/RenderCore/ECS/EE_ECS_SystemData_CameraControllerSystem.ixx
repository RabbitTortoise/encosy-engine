module;

export module EE_ECS_SystemData_CameraControllerSystem;

import EE_Encosy_Entity;
import EE_Core_WindowManager;

import <vector>;

export struct EE_CameraControllerSystemData
{
	Entity MainCamera;

	//Debug Camera Movement
	float CurrentYaw = -90;
	float CurrentPitch = 0;

	float DesiredYaw = -90;
	float DesiredPitch = 0;
};
