module;
#include <glm/gtc/quaternion.hpp>

export module Components.MaterialComponent;

import RenderCore.VulkanTypes;

export struct MaterialComponent
{
	RenderPipelineID UsedRenderPipeline;
	TextureID Texture_Albedo;
	MeshID RenderMesh;
};