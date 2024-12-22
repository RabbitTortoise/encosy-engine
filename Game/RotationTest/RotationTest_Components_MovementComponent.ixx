module;
#include <glm/glm.hpp>

export module RotationTest_Components_MovementComponent.ixx;

export struct MovementComponent
{
	glm::vec3 Direction;
	float Speed;
};