module;
#include <fmt/core.h>
#include <fmt/xchar.h>
#include <vulkan/vulkan.h>
#include <fstream>

export module RenderCore.ShaderLoader;

import RenderCore.Resources;
import RenderCore.AllocationHandler;

import <vector>;
import <map>;
import <array>;
import <filesystem>;

export enum class EngineComputeShaders	{ Gradient = 0 };
export enum class EngineVertexShaders	{ Default = 0};
export enum class EngineFragmentShaders { Default = 0};
export enum class EngineRaytracingShaders { RayGen = 0, RayClosestHit = 1, RayMiss = 2, RayShadow = 3 };

export class ShaderLoader
{
	friend class RenderCore;
	friend class RenderPipelineManager;

public:

	ShaderLoader(AllocationHandler* allocationHandler, RenderCoreResources* resources) : Resources(resources), vkAllocationHandler(allocationHandler)
	{
		std::wstring path = std::filesystem::current_path().native();;
		//fmt::println(L"Initializing ShaderLoader: Current working directory: {}",  path);

		InitEngineShaders();
	}

	~ShaderLoader() {}

	ShaderID GetEngineComputeShaderId(EngineComputeShaders shader)
	{
		return static_cast<ShaderID>(shader);
	}

	ShaderID GetEngineVertexShaderId(EngineVertexShaders shader)
	{
		return static_cast<ShaderID>(shader);
	}

	ShaderID GetEngineFragmentShaderId(EngineFragmentShaders shader)
	{
		return static_cast<ShaderID>(shader);
	}

	ShaderID LoadShader(std::string shaderFileName)
	{
		auto it = ShaderList.find(shaderFileName);
		if (it == ShaderList.end())
		{
			std::string pathName = ShaderResourceFolder + shaderFileName + ".spv";
			LoadShaderFromFile(pathName);
			size_t index = LoadedShaders.size() - 1;
			ShaderList.insert(std::pair(shaderFileName, index));
			return index;
		}
		return it->second;
	}

	VkShaderModule LoadShaderModule(std::string shaderFileName)
	{
		auto it = ShaderList.find(shaderFileName);
		if (it == ShaderList.end())
		{
			std::string pathName = ShaderResourceFolder + shaderFileName + ".spv";
			VkShaderModule loadedModule = LoadShaderFromFile(pathName);
			size_t index = LoadedShaders.size() - 1;
			ShaderList.insert(std::pair(shaderFileName, index));
			return loadedModule;
		}
		return LoadedShaders[it->second];
	}

	VkShaderModule GetShaderModuleById(ShaderID shaderID)
	{
		if (shaderID < LoadedShaders.size())
		{
			return LoadedShaders[shaderID];
		}
		fmt::println("ERROR: Shader by given id was not found!");
		return nullptr;
	}

	VkShaderModule LoadRaytracingShaderModule(std::string shaderFileName)
	{
		auto it = RaytracingShaderList.find(shaderFileName);
		if (it == RaytracingShaderList.end())
		{
			std::string pathName = ShaderResourceFolder + shaderFileName + ".spv";
			VkShaderModule loadedModule = LoadShaderFromFile(pathName);
			size_t index = LoadedRaytracingShaders.size() - 1;
			RaytracingShaderList.insert(std::pair(shaderFileName, index));
			return loadedModule;
		}
		return LoadedRaytracingShaders[it->second];
	}

	VkShaderModule GetRaytracingShaderModuleById(ShaderID shaderID)
	{
		if (shaderID < LoadedRaytracingShaders.size())
		{
			return LoadedRaytracingShaders[shaderID];
		}
		fmt::println("ERROR: Raytracing shader by given id was not found!");
		return nullptr;
	}


protected:

	void InitEngineShaders()
	{
		LoadShaderFromFile("Engine/Resources/Shaders/Gradient.comp.spv");
		LoadRaytracingShaderFromFile("Engine/Resources/Shaders/RayGeneration.rgen.spv");
		LoadRaytracingShaderFromFile("Engine/Resources/Shaders/ClosestHit.rchit.spv");
		LoadRaytracingShaderFromFile("Engine/Resources/Shaders/Miss.rmiss.spv");
		LoadRaytracingShaderFromFile("Engine/Resources/Shaders/RaytraceShadow.rmiss.spv");
	}

	void DestroyShaders()
	{
		for (auto sm : LoadedShaders)
		{
			vkDestroyShaderModule(Resources->vkDevice, sm, nullptr);
		}
		for (auto sm : LoadedRaytracingShaders)
		{
			vkDestroyShaderModule(Resources->vkDevice, sm, nullptr);
		}
	}

private:

	VkShaderModule LoadShaderFromFile(std::string shaderFilePath)
	{
		VkShaderModule loadedShader;
		bool success = ReadShaderFile(shaderFilePath, Resources->vkDevice, &loadedShader);
		if (!success)
		{
			fmt::println("ERROR: Loading Shader {} was not successful!", shaderFilePath);
		}
		else
		{
			LoadedShaders.push_back(loadedShader);
		}
		return loadedShader;
	}

	VkShaderModule LoadRaytracingShaderFromFile(std::string shaderFilePath)
	{
		VkShaderModule loadedShader;
		bool success = ReadShaderFile(shaderFilePath, Resources->vkDevice, &loadedShader);
		if (!success)
		{
			fmt::println("ERROR: Loading Shader {} was not successful!", shaderFilePath);
		}
		else
		{
			LoadedRaytracingShaders.push_back(loadedShader);
		}
		return loadedShader;
	}


	bool ReadShaderFile(std::string filePath,
		VkDevice device,
		VkShaderModule* outShaderModule)
	{
		// Open the file. With cursor at the end.
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			return false;
		}

		// Find what the size of the file is by looking up the location of the cursor
		// Because the cursor is at the end, it gives the size directly in bytes
		size_t fileSize = (size_t)file.tellg();

		// Spirv expects the buffer to be on uint32, so make sure to reserve a int vector big enough for the entire file.
		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		// Put file cursor at beginning
		file.seekg(0);

		// Load the entire file into the buffer
		file.read((char*)buffer.data(), fileSize);

		// Now that the file is loaded into the buffer, we can close it
		file.close();

		// Create a new shader module, using the buffer we loaded
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;

		// CodeSize has to be in bytes, so multply the ints in the buffer by size of int to know the real size of the buffer
		createInfo.codeSize = buffer.size() * sizeof(uint32_t);
		createInfo.pCode = buffer.data();

		// Check that the creation goes well.
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			return false;
		}
		*outShaderModule = shaderModule;
		return true;
	}

	std::vector<VkShaderModule> LoadedShaders;
	std::vector<VkShaderModule> LoadedRaytracingShaders;
	std::map<std::string, ShaderID> ShaderList;
	std::map<std::string, ShaderID> RaytracingShaderList;

	RenderCoreResources* Resources;
	AllocationHandler* vkAllocationHandler;

	std::string ShaderResourceFolder = "Game/Resources/Shaders/";
};
