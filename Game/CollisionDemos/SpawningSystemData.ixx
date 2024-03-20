module;
#include <glm/vec3.hpp>

export module Demo.SystemData.SpawningSystem;

import EncosyCore.Entity;
import Components.MaterialComponent;
import <vector>;

export struct SpawningSystemData
{
	std::vector<Entity> CurrentLeaders;
	std::vector<int> CurrentLeaderIndexes;
	std::vector<MaterialComponentLit> LeaderMaterial;
	glm::vec3 PlayRegionMin = { -100, -100, -100 };
	glm::vec3 PlayRegionMax = { -100, -100, -100 };
};
