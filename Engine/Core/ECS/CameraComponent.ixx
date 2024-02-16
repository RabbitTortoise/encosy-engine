module;
#include <glm/glm.hpp>

export module Components.CameraComponent;

export struct CameraComponent
{
	float Fov;
	float NearPlane;
	float FarPlane;

	glm::vec3 Front;
	glm::vec3 Right;
	glm::vec3 Up;

	glm::mat4 Orientation;
	glm::mat4 ProjectionMatrix;
};
