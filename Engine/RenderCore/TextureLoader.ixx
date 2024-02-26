module;
#include <vulkan/vulkan.h>
#include <fmt/core.h>
#include <fmt/xchar.h>
#include <array>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

export module RenderCore.TextureLoader;

import RenderCore.VulkanTypes;
import RenderCore.AllocationHandler;

import <vector>;
import <map>;
import <filesystem>;

export enum class EngineTextures { ErrorCheckerBoard = 0, White, Black };

export class TextureLoader
{

	friend class RenderCore;

public:

	TextureLoader(AllocationHandler* allocationHandler) : vkAllocationHandler(allocationHandler) 
	{
		std::wstring path = std::filesystem::current_path().native();;
		fmt::println(L"Initializing TextureLoader: Current working directory: {}", path);
	
		InitEngineTextures();
	}
	~TextureLoader() {}

	TextureID GetEngineTextureID(EngineTextures texture)
	{
		return static_cast<size_t>(texture);
	}

	TextureID LoadTexture(std::string textureName)
	{
		auto it = TextureList.find(textureName);
		if (it == TextureList.end())
		{
			LoadTextureFromFile(textureName);
			it = TextureList.find(textureName);
			return it->second;
		}
		else
		{
			return it->second;
		}
	}

	AllocatedImage GetEngineTexture(EngineTextures texture)
	{
		return TextureHandles[static_cast<int>(texture)];
	}

	AllocatedImage GetTexture(TextureID index)
	{
		return TextureHandles[index];
	}

protected:

	AllocatedImage LoadTextureFromFile(std::string textureName)
	{
		// Replace with TextureResourceFolder
		std::string filePath = TextureResourceFolder + textureName;
		auto it = TextureList.find(filePath);
		if (it == TextureList.end())
		{
			//stbi_set_flip_vertically_on_load(true);
			// Load the data for our texture using stb-image stbi_load-function
			int width, height, nrChannels;
			stbi_uc* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
			AllocatedImage newImage;
			int index = 0;
			if (data)
			{
				newImage = TextureHandles.emplace_back(CreateTexture(width, height, data));
				index = TextureHandles.size() - 1;
				stbi_image_free(data);
			}
			else
			{
				//If loading was not successfull use error texture
				fmt::println("ERROR: Loading {} was not successfull", textureName);
				newImage = GetEngineTexture(EngineTextures::ErrorCheckerBoard);
				index = static_cast<int>(EngineTextures::ErrorCheckerBoard);
			}

			TextureList.insert(std::pair(textureName, index));
			return newImage;
		}
		else
		{
			return TextureHandles[it->second];
		}
	}

	/*
	To be implemented, this is old code from previous project
	AllocatedImage LoadHDRTextureFromFile(std::string textureName)
	{
		std::string filePath = TextureResourceFolder + textureName;

		auto it = HDRTextureList.find(filePath);
		if (it == HDRTextureList.end())
		{
			stbi_set_flip_vertically_on_load(true);
			int width, height, nrComponents;
			float* data = stbi_loadf(filePath.c_str(), &width, &height, &nrComponents, 0);
			AllocatedImage newImage = HDRTextureHandles.emplace_back(CreateHDRTexture(width, height, nrComponents, data));

			int index = HDRTextureHandles.size() - 1;
			HDRTextureList.insert(std::pair(filePath, index));
			return newImage;
		}
		else
		{
			return HDRTextureHandles[it->second];
		}
	}
	TextureID LoadHDRTexture(std::string textureName)
	{
		auto it = HDRTextureList.find(textureName);
		if (it == HDRTextureList.end())
		{
			LoadHDRTextureFromFile(textureName);
			it = HDRTextureList.find(textureName);
			return it->second;
		}
		else
		{
			return it->second;
		}
	}
	*/



private:
	void InitEngineTextures()
	{
		if (TextureHandles.size() == 0)
		{
			uint32_t white = 0xFFFFFFFF;
			uint32_t black = 0xFF000000;
			uint32_t magenta = 0xFFFF00FF;
			uint32_t grey = 0xFFAAAAAA;

			// Checkerboard image
			std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
			for (int x = 0; x < 16; x++) {
				for (int y = 0; y < 16; y++) {
					pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
				}
			}

			//3 default textures, white, grey, black. 1 pixel each
			AllocatedImage errorCheckerboardImage = vkAllocationHandler->AllocateAndUploadImageToGPU(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
			AllocatedImage whiteImage = vkAllocationHandler->AllocateAndUploadImageToGPU((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
			AllocatedImage blackImage = vkAllocationHandler->AllocateAndUploadImageToGPU((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
		
			TextureHandles.push_back(errorCheckerboardImage);
			TextureHandles.push_back(whiteImage);
			TextureHandles.push_back(blackImage);
		}
	}

	// Currently always assumes that there is 4 channels and the format is always VK_FORMAT_R8G8B8A8_UNORM
	AllocatedImage CreateTexture(uint32_t width, uint32_t height, stbi_uc* data)
	{
		return vkAllocationHandler->AllocateAndUploadImageToGPU(data, VkExtent3D{ width, height, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
	}

	std::vector<AllocatedImage> TextureHandles;
	std::map<std::string, TextureID> TextureList;

	AllocationHandler* vkAllocationHandler;

	std::string TextureResourceFolder = "Game/Resources/Textures/";
};
