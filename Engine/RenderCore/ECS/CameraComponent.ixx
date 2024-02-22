module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module Components.CameraComponent;

export struct CameraComponent
{
	float Fov;
	float NearPlane;
	float FarPlane;

	glm::vec3 Front;
	glm::vec3 Right;
	glm::vec3 Up;

	glm::mat4 View;
	glm::mat4 Projection;
};
