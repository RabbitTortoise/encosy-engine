module;
#include <glm/gtc/quaternion.hpp>

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
