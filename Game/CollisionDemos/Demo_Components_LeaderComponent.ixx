module;
#include <glm/vec3.hpp>

export module Demo_Components_LeaderComponent;

export struct LeaderComponent
{
	int LeaderID = 0;
	glm::vec3 TargetPoint;
};
