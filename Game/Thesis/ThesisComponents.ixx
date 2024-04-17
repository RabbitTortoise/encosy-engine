module;
#include <glm/glm.hpp>

export module Thesis.Components;

export struct RotateComponent
{
	glm::vec3 Direction;
	float Speed;
};

export struct EntitySpawningSystemData
{
	glm::vec3 SpawnRegionMin = { -100, -100, -100 };
	glm::vec3 SpawnRegionMax = { -100, -100, -100 };
};


export struct DestroyEntityComponent {};

export struct EntityTypeComponent1 {};
export struct EntityTypeComponent2 {};
export struct EntityTypeComponent3 {};
export struct EntityTypeComponent4 {};
export struct EntityTypeComponent5 {};
export struct EntityTypeComponent6 {};
export struct EntityTypeComponent7 {};
export struct EntityTypeComponent8 {};
export struct EntityTypeComponent9 {};
export struct EntityTypeComponent10 {};
