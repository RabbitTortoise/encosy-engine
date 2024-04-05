module;
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

export module Components.MaterialComponent;

import RenderCore.VulkanTypes;

export struct MaterialComponentUnlit
{
	TextureID Diffuse;
	MeshID RenderMesh;
	float TextureRepeat;
};

export struct MaterialComponentLit
{
	TextureID Diffuse;
	TextureID Normal;
	MeshID RenderMesh;
	float TextureRepeat;
	glm::vec3 Color = glm::vec3(1,1,1);
};

export struct MaterialComponentPBR
{
	TextureID Albedo;
	TextureID Normal;
	TextureID Depth;
	TextureID Metallic;
	TextureID Roughness;
	TextureID AO;
	MeshID RenderMesh;
	float TextureRepeat;
};
