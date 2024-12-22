module;
#include <glm/glm.hpp>

export module EncosyGame_DemoCommon;

import EE_RenderCore_TextureLoader;
import EE_RenderCore_VulkanTypes;
import EncosyEngine_RenderCore;

import <vector>;

export
void CreateTextures(TextureLoader* MainTextureLoader, RenderCore* EngineRenderCore, std::vector<PBRTextureSet>& sets, std::vector<TextureSetID>& setIds)
{
	auto whiteID = MainTextureLoader->GetEngineTextureID(EngineTextures::White);
	auto blackID = MainTextureLoader->GetEngineTextureID(EngineTextures::Black);
	auto greyID = MainTextureLoader->GetEngineTextureID(EngineTextures::Grey);
	auto normalID = MainTextureLoader->GetEngineTextureID(EngineTextures::NeutralNormal);

	auto grassID = MainTextureLoader->LoadTexture("Grass_Texture.png");
	auto rockID = MainTextureLoader->LoadTexture("Rock_Texture.png");
	auto sandID = MainTextureLoader->LoadTexture("Sand_Texture.png");
	auto snowID = MainTextureLoader->LoadTexture("Snow_Texture.png");
	auto waterID = MainTextureLoader->LoadTexture("Water_Texture.png");
	auto grassNormalID = MainTextureLoader->LoadTexture("Grass_Normal.png");
	auto rockNormalID = MainTextureLoader->LoadTexture("Rock_Normal.png");
	auto sandNormalID = MainTextureLoader->LoadTexture("Sand_Normal.png");
	auto snowNormalID = MainTextureLoader->LoadTexture("Snow_Normal.png");
	auto waterNormalID = MainTextureLoader->LoadTexture("Water_Normal.png");

	std::vector<TextureID> textureIds;
	textureIds.push_back(grassID);
	textureIds.push_back(rockID);
	textureIds.push_back(sandID);
	textureIds.push_back(snowID);
	textureIds.push_back(waterID);

	PBRTextureSet grassSet = PBRTextureSet(grassID, whiteID, blackID, blackID, grassNormalID, blackID);
	PBRTextureSet rockSet = PBRTextureSet(rockID, whiteID, blackID, blackID, rockNormalID, blackID);
	PBRTextureSet sandSet = PBRTextureSet(sandID, whiteID, blackID, blackID, sandNormalID, blackID);
	PBRTextureSet snowSet = PBRTextureSet(snowID, whiteID, blackID, blackID, snowNormalID, blackID);
	PBRTextureSet waterSet = PBRTextureSet(waterID, whiteID, blackID, blackID, waterNormalID, blackID);
	std::vector<PBRTextureSet> createdTextures;
	createdTextures.push_back(grassSet);
	createdTextures.push_back(rockSet);
	createdTextures.push_back(sandSet);
	createdTextures.push_back(snowSet);
	createdTextures.push_back(waterSet);
	sets = createdTextures;

	auto grassSetID = EngineRenderCore->RegisterTextureSetForRaytracingUsage(grassSet);
	auto rockSetID = EngineRenderCore->RegisterTextureSetForRaytracingUsage(rockSet);
	auto sandSetID = EngineRenderCore->RegisterTextureSetForRaytracingUsage(sandSet);
	auto snowSetID = EngineRenderCore->RegisterTextureSetForRaytracingUsage(snowSet);
	auto waterSetID = EngineRenderCore->RegisterTextureSetForRaytracingUsage(waterSet);
	std::vector<TextureSetID> createdSetIds;
	createdSetIds.push_back(grassSetID);
	createdSetIds.push_back(rockSetID);
	createdSetIds.push_back(sandSetID);
	createdSetIds.push_back(snowSetID);
	createdSetIds.push_back(waterSetID);
	setIds = createdSetIds;

	RenderCoreResources* CoreResources = EngineRenderCore->GetRenderCoreResources();
}
