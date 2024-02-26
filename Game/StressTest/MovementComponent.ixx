module;
#include <glm/glm.hpp>

export module StressTest.Components.MovementComponent;

export struct MovementComponent
{
	glm::vec3 Direction;
	float Speed;
};