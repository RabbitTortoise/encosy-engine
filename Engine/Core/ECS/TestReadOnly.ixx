module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module Components.TestReadOnlyComponent;

export struct TestReadOnlyComponent
{
	glm::vec3 Position;
	glm::vec3 Scale;
	glm::vec3 EulerAngles;
	glm::quat Orientation;
};