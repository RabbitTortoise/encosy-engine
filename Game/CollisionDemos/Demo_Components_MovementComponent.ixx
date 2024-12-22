module;
#include <glm/glm.hpp>

export module Demo_Components_MovementComponent;

export struct MovementComponent
{
	glm::vec3 Direction;
	float Speed;
};