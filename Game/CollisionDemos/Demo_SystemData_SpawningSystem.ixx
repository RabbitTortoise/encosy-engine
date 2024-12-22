module;
#include <glm/vec3.hpp>

export module Demo_SystemData_SpawningSystem;

import EE_Encosy_Entity;
import EE_ECS_Components_MaterialComponent;
import EE_RenderCore_VulkanTypes;
import <vector>;

export struct SpawningSystemData
{
	std::vector<Entity> CurrentLeaders;
	std::vector<int> CurrentLeaderIndexes;
	std::vector<EE_MaterialComponent> LeaderMaterial;
	glm::vec3 PlayRegionMin = { -100, -100, -100 };
	glm::vec3 PlayRegionMax = { -100, -100, -100 };
	float minFPS = 60;
	float resetFPS = 60;

	std::vector<PBRTextureSet> textureSets;
};
