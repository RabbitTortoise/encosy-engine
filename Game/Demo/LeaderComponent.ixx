module;
#include <glm/vec3.hpp>

export module Demo.Components.LeaderComponent;

export struct LeaderComponent
{
	int LeaderID = 0;
	glm::vec3 TargetPoint;
};
