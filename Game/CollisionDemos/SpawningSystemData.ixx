module;
#include <glm/vec3.hpp>

export module Demo.SystemData.SpawningSystem;

import EncosyCore.Entity;
import Components.MaterialComponent;
import RenderCore.VulkanTypes;
import <vector>;

export struct SpawningSystemData
{
	std::vector<Entity> CurrentLeaders;
	std::vector<int> CurrentLeaderIndexes;
	std::vector<MaterialComponentRaytracing> LeaderMaterial;
	glm::vec3 PlayRegionMin = { -100, -100, -100 };
	glm::vec3 PlayRegionMax = { -100, -100, -100 };
	float minFPS = 60;
	float resetFPS = 60;

	std::vector<PBRTextureSet> textureSets;
};
