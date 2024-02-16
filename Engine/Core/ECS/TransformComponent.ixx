module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module Components.TransformComponent;

export struct TransformComponent
{
	glm::vec3 Position;
	glm::vec3 Scale;
	glm::quat Orientation;
};