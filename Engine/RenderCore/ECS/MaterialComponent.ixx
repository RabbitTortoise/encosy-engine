module;
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

export module Components.MaterialComponent;

import RenderCore.VulkanTypes;

export struct MaterialComponentRaytracing
{
	TextureID TextureSet;
	MeshID RenderMesh;
	float TextureRepeat;
	glm::vec3 Color = glm::vec3(1, 1, 1);
};
