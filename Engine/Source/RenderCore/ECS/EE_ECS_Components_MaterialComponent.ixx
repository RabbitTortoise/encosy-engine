module;
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

export module EE_ECS_Components_MaterialComponent;

import EE_RenderCore_VulkanTypes;

export struct EE_MaterialComponentRaytracing
{
	TextureID TextureSet;
	MeshID RenderMesh;
	float TextureRepeat;
	glm::vec3 Color = glm::vec3(1, 1, 1);
};
