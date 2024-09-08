module;

#include <glm/glm.hpp>	
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <fmt/core.h>
#include <fmt/xchar.h>

export module EE_RenderCore_MeshLoader;

import EE_RenderCore_VulkanTypes;
import EE_RenderCore_AllocationHandler;

import <vector>;
import <memory>;
import <span>;
import <map>;
import <filesystem>;

export enum class EngineMesh { Error = 0, Cube, Quad, Sphere, Torus };


export class MeshLoader
{
	friend class RenderCore;

public:

	MeshLoader(AllocationHandler* allocationHandler) : vkAllocationHandler(allocationHandler) 
	{
		std::wstring path = std::filesystem::current_path().native();
		//fmt::println(L"Initializing MeshLoader: Current working directory: {}", path);

		InitPrimitives();
	}
	~MeshLoader() {}

	void InitPrimitives()
	{
		if (PrimitivesList.size() == 0)
		{
			std::vector<size_t> meshIDs = LoadMeshesFromFile("Engine/Resources/Models/error.obj", "Engine/Resources/Models/error.obj");
			CreateMeshBuffersToGPU(meshIDs);
			PrimitivesList.emplace(std::pair<EngineMesh, int>(EngineMesh::Error, static_cast<int>(meshIDs[0])));

			meshIDs = LoadMeshesFromFile("Engine/Resources/Models/cube.obj", "Engine/Resources/Models/cube.obj");
			CreateMeshBuffersToGPU(meshIDs);
			PrimitivesList.emplace(std::pair<EngineMesh, int>(EngineMesh::Cube, static_cast<int>(meshIDs[0])));

			meshIDs = LoadMeshesFromFile("Engine/Resources/Models/quad.obj", "Engine/Resources/Models/quad.obj");
			CreateMeshBuffersToGPU(meshIDs);
			PrimitivesList.emplace(std::pair<EngineMesh, int>(EngineMesh::Quad, static_cast<int>(meshIDs[0])));

			meshIDs = LoadMeshesFromFile("Engine/Resources/Models/sphere.obj", "Engine/Resources/Models/sphere.obj");
			CreateMeshBuffersToGPU(meshIDs);
			PrimitivesList.emplace(std::pair<EngineMesh, int>(EngineMesh::Sphere, static_cast<int>(meshIDs[0])));

			meshIDs = LoadMeshesFromFile("Engine/Resources/Models/torus.obj", "Engine/Resources/Models/torus.obj");
			CreateMeshBuffersToGPU(meshIDs);
			PrimitivesList.emplace(std::pair<EngineMesh, int>(EngineMesh::Torus, static_cast<int>(meshIDs[0])));
		}
	}

	MeshID GetMeshID(std::string modelFileName)
	{
		std::vector<MeshID> meshIDs;
		auto it = MeshList.find(modelFileName);

		// If requested file has not been loaded, load it and setup meshes for usage.
		if (it == MeshList.end())
		{
			meshIDs = LoadModelFromFile(modelFileName);
			if (meshIDs.empty())
			{
				// ERROR: FILE NOT READABLE
				return static_cast<MeshID>(EngineMesh::Error);
			}
			else
			{
				CreateMeshBuffersToGPU(meshIDs);
				return meshIDs[0];
			}
		}
		// If the requested model has already been loaded, search for the meshes that were loaded before and return them.
		else
		{
			return it->second;
		}
	}

	MeshID GetEngineMeshID(EngineMesh shape)
	{
		return static_cast<MeshID>(shape);
	}

	const Mesh* GetMesh(MeshID id)
	{
		return Meshes[id].MeshInfo.get();

	}

	GPUMeshBuffers* GetMeshBuffers(MeshID id)
	{
		return Meshes[id].BufferInfo.get();
	}

	GPUMeshBuffers* GetRaytracingMeshBuffers(MeshID id)
	{
		return Meshes[id].BufferInfo.get();
	}

	MeshAllocatedData GetMeshAllocatedData(MeshID id)
	{
		const MeshDataStorage& mesh = Meshes[id];
		MeshAllocatedData data =
		{
			.MeshInfo = mesh.MeshInfo.get(),
			.BufferInfo = mesh.BufferInfo.get(),
			.RaytracingBufferInfo = mesh.RaytracingBufferInfo.get()
		};
		return data;
	}

	std::vector<MeshAllocatedData> GetAllMeshAllocatedData()
	{
		std::vector<MeshAllocatedData> returnVec;
		returnVec.reserve(Meshes.size());
		for(const MeshDataStorage& mesh : Meshes)
		{
			MeshAllocatedData data =
			{
				.MeshInfo = mesh.MeshInfo.get(),
				.BufferInfo = mesh.BufferInfo.get(),
				.RaytracingBufferInfo = mesh.RaytracingBufferInfo.get()
			};
			returnVec.push_back(data);
		}
		return returnVec;
	}

protected:

	std::vector<MeshID> LoadModelFromFile(std::string modelFileName)
	{
		std::string modelFilePath = ModelResourceFolder + modelFileName;
		std::vector<MeshID>  meshIDs = LoadMeshesFromFile(modelFilePath, modelFileName);
		return meshIDs;
	}

	const GPUMeshBuffers* GetPrimitiveBuffer(EngineMesh shape)
	{
		auto it = PrimitivesList.find(shape);
		return Meshes[it->second].BufferInfo.get();
	}
	const Mesh* GetPrimitiveMesh(EngineMesh shape)
	{
		auto it = PrimitivesList.find(shape);
		return Meshes[it->second].MeshInfo.get();
	}

	std::vector<size_t> LoadMeshesFromFile(std::string modelFilePath, std::string modelFileName)
	{
		std::vector<size_t> meshIDs;

		std::vector<Mesh*> loadedMeshes = LoadMeshes(modelFilePath);


		if (loadedMeshes.size() > 0)
		{
			for (size_t i = 0; i < loadedMeshes.size(); i++)
			{
				Mesh* m = loadedMeshes[i];
				MeshDataStorage meshData;
				meshData.MeshInfo = std::make_unique<Mesh>();
				meshData.MeshInfo.reset(m);

				int meshID = static_cast<int>(Meshes.size());
				meshIDs.push_back(meshID);
				Meshes.emplace_back(std::move(meshData));

				if (i == 0) { MeshList.emplace(std::pair<std::string, int>(modelFileName, meshID)); }
				else { MeshList.emplace(std::pair<std::string, int>((modelFileName + "::" + std::to_string(i)), meshID)); }
			}
		}
		
		return meshIDs;
	}


private:
	Mesh* ProcessAssimpMesh(aiMesh* mesh)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		//std::vector<Texture> textures;	//Textures not yet captured from the model.
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;

			glm::vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.position = vector;

			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.normal = vector;

			if (mesh->GetNumUVChannels() > 0)
			{
				glm::vec2 vec;
				vertex.uv_x = mesh->mTextureCoords[0][i].x;
				vertex.uv_y = mesh->mTextureCoords[0][i].y;
			}
			if (mesh->HasTangentsAndBitangents())
			{
				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;
				vertex.tangent = vector;
			}
			if (mesh->HasVertexColors(0))
			{
				vertex.color.r = mesh->mColors[0]->r;
				vertex.color.g = mesh->mColors[0]->g;
				vertex.color.b = mesh->mColors[0]->b;
				vertex.color.a = mesh->mColors[0]->a;
			}
			else
			{
				vertex.color.r = 1;
				vertex.color.g = 1;
				vertex.color.b = 1;
				vertex.color.a = 1;
			}

			vertices.push_back(vertex);
		}
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}
		Mesh* m = new Mesh();
		m->vertices = vertices;
		m->indices = indices;
		return m;

	}
	void ProcessAssimpNode(std::vector<Mesh*>* meshes, aiNode* node, const aiScene* scene)
	{
		// Process each mesh located at the current node
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			// the node object only contains indices to index the actual objects in the scene.
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes->push_back(ProcessAssimpMesh(mesh));
		}
		// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessAssimpNode(meshes, node->mChildren[i], scene);
		}
	}

	std::vector<Mesh*> LoadMeshes(const std::string& path)
	{
		std::vector<Mesh*> meshes;
		// Read file with Assimp
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		// Check for errors
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			fmt::println("Error loading model file {}: {}", path, importer.GetErrorString());
			return meshes;
		}

		ProcessAssimpNode(&meshes, scene->mRootNode, scene);

		return meshes;
	}
	void CreateMeshBuffersToGPU(std::span<size_t> meshHandles)
	{
		for (auto h : meshHandles)
		{
			GPUMeshBuffers bufferInfo = vkAllocationHandler->UploadMeshToGPU(Meshes[h].MeshInfo->vertices, Meshes[h].MeshInfo->indices);

			//Create vertex position vector for raytracing
			std::vector<VertexPos> raytracingMesh;
			raytracingMesh.reserve(Meshes[h].MeshInfo->vertices.size());
			for(const auto& v : Meshes[h].MeshInfo->vertices)
			{
				VertexPos nv = { glm::vec3(v.position.x, v.position.y, v.position.z) };
				raytracingMesh.emplace_back(nv);
			}

			GPURaytracingMeshBuffer raytracingBufferInfo = vkAllocationHandler->UploadMeshForRaytracingToGPU(raytracingMesh, Meshes[h].MeshInfo->indices);
			Meshes[h].BufferInfo = std::make_unique<GPUMeshBuffers>(bufferInfo);
			Meshes[h].RaytracingBufferInfo = std::make_unique<GPURaytracingMeshBuffer>(raytracingBufferInfo);
			Meshes[h].UploadedToGPU = true;
		}
	}

	struct MeshDataStorage
	{
		std::unique_ptr<Mesh> MeshInfo;
		std::unique_ptr<GPUMeshBuffers> BufferInfo;
		std::unique_ptr<GPURaytracingMeshBuffer> RaytracingBufferInfo;
		bool UploadedToGPU = false;
	};

	std::vector<MeshDataStorage> Meshes;
	std::map<std::string, MeshID> MeshList;

	std::map<EngineMesh, MeshID> PrimitivesList;
	AllocationHandler* vkAllocationHandler;

	std::string ModelResourceFolder = "Game/Resources/Models/";

};