module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EE_ECS_Components_TransformComponent;

export struct EE_TransformComponent
{
	glm::vec3 Position;
	glm::vec3 Scale;
	glm::quat Orientation;
};