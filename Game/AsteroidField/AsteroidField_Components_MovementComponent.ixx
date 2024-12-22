module;
#include <glm/glm.hpp>

export module AsteroidField_Components_MovementComponent;

export struct AsteroidFieldMovementComponent
{
	glm::vec3 goalPosition;
	glm::vec2 prevAngle;
	glm::vec2 goalAngle;
	glm::vec2 angleDirection;
	float speed;
	float radius;
};