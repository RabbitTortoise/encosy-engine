module;
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

export module EE_ECS_Components_CameraComponent;

export struct EE_CameraComponent
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
