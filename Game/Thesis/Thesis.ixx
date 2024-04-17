module;
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EncosyGame.Thesis;

import EncosyEngine.Interface;
import EncosyEngine.EncosyCore;
import EncosyEngine.RenderCore;

import Components.TransformComponent;
import Components.MaterialComponent;
import Components.ModelMatrixComponent;
import Thesis.Components;

import Thesis.Systems.ThesisEntitySpawningSystemSt1;
import Thesis.Systems.ThesisEntitySpawningSystemSt2;
import Thesis.Systems.ThesisEntitySpawningSystemSt3;
import Thesis.Systems.ThesisEntitySpawningSystemSt4;
import Thesis.Systems.ThesisEntitySpawningSystemSt5;
import Thesis.Systems.ThesisEntitySpawningSystemSt6;
import Thesis.Systems.ThesisEntitySpawningSystemSt7;
import Thesis.Systems.ThesisEntitySpawningSystemSt8;
import Thesis.Systems.ThesisEntitySpawningSystemSt9;
import Thesis.Systems.ThesisEntitySpawningSystemSt10;
import Thesis.Systems.ThesisEntityModifySystemSt1;
import Thesis.Systems.ThesisEntityModifySystemSt2;
import Thesis.Systems.ThesisEntityModifySystemSt3;
import Thesis.Systems.ThesisEntityModifySystemSt4;
import Thesis.Systems.ThesisEntityModifySystemSt5;
import Thesis.Systems.ThesisEntityModifySystemSt6;
import Thesis.Systems.ThesisEntityModifySystemSt7;
import Thesis.Systems.ThesisEntityModifySystemSt8;
import Thesis.Systems.ThesisEntityModifySystemSt9;
import Thesis.Systems.ThesisEntityModifySystemSt10;
import Thesis.Systems.ThesisEntityDestroySystemSt1;
import Thesis.Systems.ThesisEntityDestroySystemSt2;
import Thesis.Systems.ThesisEntityDestroySystemSt3;
import Thesis.Systems.ThesisEntityDestroySystemSt4;
import Thesis.Systems.ThesisEntityDestroySystemSt5;
import Thesis.Systems.ThesisEntityDestroySystemSt6;
import Thesis.Systems.ThesisEntityDestroySystemSt7;
import Thesis.Systems.ThesisEntityDestroySystemSt8;
import Thesis.Systems.ThesisEntityDestroySystemSt9;
import Thesis.Systems.ThesisEntityDestroySystemSt10;
import Thesis.Systems.ThesisRotationSystemSt;
import Thesis.Systems.ThesisModelMatrixBuilderSystemSt;


import Thesis.Systems.ThesisModelMatrixBuilderSystemMt;
import Thesis.Systems.ThesisRotationSystemMt;
import Thesis.Systems.ThesisEntityDestroySystemMt1;
import Thesis.Systems.ThesisEntityDestroySystemMt2;
import Thesis.Systems.ThesisEntityDestroySystemMt3;
import Thesis.Systems.ThesisEntityDestroySystemMt4;
import Thesis.Systems.ThesisEntityDestroySystemMt5;
import Thesis.Systems.ThesisEntityDestroySystemMt6;
import Thesis.Systems.ThesisEntityDestroySystemMt7;
import Thesis.Systems.ThesisEntityDestroySystemMt8;
import Thesis.Systems.ThesisEntityDestroySystemMt9;
import Thesis.Systems.ThesisEntityDestroySystemMt10;
import Thesis.Systems.ThesisEntitySpawningSystemMt1;
import Thesis.Systems.ThesisEntitySpawningSystemMt2;
import Thesis.Systems.ThesisEntitySpawningSystemMt3;
import Thesis.Systems.ThesisEntitySpawningSystemMt4;
import Thesis.Systems.ThesisEntitySpawningSystemMt5;
import Thesis.Systems.ThesisEntitySpawningSystemMt6;
import Thesis.Systems.ThesisEntitySpawningSystemMt7;
import Thesis.Systems.ThesisEntitySpawningSystemMt8;
import Thesis.Systems.ThesisEntitySpawningSystemMt9;
import Thesis.Systems.ThesisEntitySpawningSystemMt10;
import Thesis.Systems.ThesisEntityModifySystemMt1;
import Thesis.Systems.ThesisEntityModifySystemMt2;
import Thesis.Systems.ThesisEntityModifySystemMt3;
import Thesis.Systems.ThesisEntityModifySystemMt4;
import Thesis.Systems.ThesisEntityModifySystemMt5;
import Thesis.Systems.ThesisEntityModifySystemMt6;
import Thesis.Systems.ThesisEntityModifySystemMt7;
import Thesis.Systems.ThesisEntityModifySystemMt8;
import Thesis.Systems.ThesisEntityModifySystemMt9;
import Thesis.Systems.ThesisEntityModifySystemMt10;


import RenderCore.MeshLoader;
import RenderCore.TextureLoader;
import RenderCore.RenderPipelineManager;

import <vector>;
import <random>;

void InitThesisTestCommon(int entityCountPerType)
{

	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

	auto EngineCore = EncosyEngine::GetEncosyCore();
	auto PrimaryWorld = EngineCore->GetPrimaryWorld();

	auto WorldComponentManager = PrimaryWorld->GetWorldComponentManager();
	auto WorldEntityManager = PrimaryWorld->GetWorldEntityManager();
	auto WorldSystemManager = PrimaryWorld->GetWorldSystemManager();

	auto EngineRenderCore = EncosyEngine::GetRenderCore();
	auto MainTextureLoader = EngineRenderCore->GetTextureLoader();
	auto MainMeshLoader = EngineRenderCore->GetMeshLoader();
	auto MainRenderPipelineManager = EngineRenderCore->GetRenderPipelineManager();


	EntitySpawningSystemData spawningData =
	{
		.SpawnRegionMin = { -100,-100,-100 },
		.SpawnRegionMax = { 100,100,100 }
	};
	WorldComponentManager->CreateSystemDataStorage(spawningData);

	auto entityType1 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent1>("EntityType1");
	auto entityToDestroy1 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent1>("EntityToDestroy1");

	auto entityType2 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent2>("EntityType2");
	auto entityToDestroy2 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent2>("EntityToDestroy2");

	auto entityType3 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent3>("EntityType3");
	auto entityToDestroy3 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent3>("EntityToDestroy3");

	auto entityType4 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent4>("EntityType4");
	auto entityToDestroy4 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent4>("EntityToDestroy4");

	auto entityType5 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent5>("EntityType5");
	auto entityToDestroy5 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent5>("EntityToDestroy5");

	auto entityType6 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent6>("EntityType6");
	auto entityToDestroy6 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent6>("EntityToDestroy6");

	auto entityType7 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent7>("EntityType7");
	auto entityToDestroy7 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent7>("EntityToDestroy7");

	auto entityType8 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent8>("EntityType8");
	auto entityToDestroy8 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent8>("EntityToDestroy8");

	auto entityType9 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent9>("EntityType9");
	auto entityToDestroy9 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent9>("EntityToDestroy9");

	auto entityType10 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, RotateComponent,
		EntityTypeComponent10>("EntityType10");
	auto entityToDestroy10 = WorldEntityManager->CreateEntityType<TransformComponent, MaterialComponentLit, ModelMatrixComponent, DestroyEntityComponent,
		EntityTypeComponent10>("EntityToDestroy10");



	std::random_device rd;
	std::mt19937 Gen(rd());
	std::uniform_real_distribution<float> distrX(spawningData.SpawnRegionMin.x, spawningData.SpawnRegionMax.x);
	std::uniform_real_distribution<float> distrY(spawningData.SpawnRegionMin.y, spawningData.SpawnRegionMax.y);
	std::uniform_real_distribution<float> distrZ(spawningData.SpawnRegionMin.z, spawningData.SpawnRegionMax.z);
	std::uniform_real_distribution<float> distr01(0.0f, 1.0f);

	auto grassID = MainTextureLoader->LoadTexture("Grass_Texture.png");
	auto grassNormalID = MainTextureLoader->LoadTexture("Grass_Normal.png");
	auto rockID = MainTextureLoader->LoadTexture("Rock_Texture.png");
	auto rockNormalID = MainTextureLoader->LoadTexture("Rock_Normal.png");
	auto sandID = MainTextureLoader->LoadTexture("Sand_Texture.png");
	auto sandNormalID = MainTextureLoader->LoadTexture("Sand_Normal.png");
	auto snowID = MainTextureLoader->LoadTexture("Snow_Texture.png");
	auto snowNormalID = MainTextureLoader->LoadTexture("Snow_Normal.png");
	auto waterID = MainTextureLoader->LoadTexture("Water_Texture.png");
	auto waterNormalID = MainTextureLoader->LoadTexture("Water_Normal.png");

	std::vector<TextureID> textureIds;
	textureIds.push_back(grassID);
	textureIds.push_back(rockID);
	textureIds.push_back(sandID);
	textureIds.push_back(snowID);
	textureIds.push_back(waterID);

	std::vector<MeshID> meshIDs;
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Cube));
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Sphere));
	meshIDs.push_back(MainMeshLoader->GetEngineMeshID(EngineMesh::Torus));


	EntityTypeComponent1 et1;
	EntityTypeComponent2 et2;
	EntityTypeComponent3 et3;
	EntityTypeComponent4 et4;
	EntityTypeComponent5 et5;
	EntityTypeComponent6 et6;
	EntityTypeComponent7 et7;
	EntityTypeComponent9 et8;
	EntityTypeComponent9 et9;
	EntityTypeComponent10 et10;

	for (size_t type = 0; type < 10; type++)
	{
		for (size_t c = 0; c < entityCountPerType; c++)
		{
			glm::vec3 randPos = glm::vec3(distrX(Gen), distrY(Gen), distrZ(Gen));
			float rand1 = distr01(Gen);
			float rand2 = distr01(Gen);
			float rand3 = distr01(Gen);
			int textureSelect = std::round(rand1 * (textureIds.size() - 1));
			int meshSelect = std::round(rand2 * (meshIDs.size() - 1));

			TransformComponent tc =
			{
				.Position = randPos,
				.Scale = {1,1,1},
				.Orientation = glm::quat(glm::vec3(0,0,0)),
			};

			TextureID usedTextureID = textureIds[textureSelect];
			MeshID usedMeshId = meshIDs[meshSelect];
			MaterialComponentLit mc =
			{
				.Diffuse = usedTextureID,
				.Normal = usedTextureID + 1,
				.RenderMesh = usedMeshId,
				.TextureRepeat = 1.0f,
				.Color = glm::vec3(1,1,1)
			};
			ModelMatrixComponent matrix = {};

			DestroyEntityComponent dec;

			Entity createdEntity;

			switch (type)
			{
			case 0:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et1, matrix);
				break;
			case 1:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et2, matrix);
				break;
			case 2:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et3, matrix);
				break;
			case 3:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et4, matrix);
				break;
			case 4:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et5, matrix);
				break;
			case 5:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et6, matrix);
				break;
			case 6:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et7, matrix);
				break;
			case 7:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et8, matrix);
				break;
			case 8:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et9, matrix);
				break;
			case 9:
				createdEntity = WorldEntityManager->CreateEntityWithData(tc, mc, dec, et10, matrix);
				break;
			default:
				break;
			}
		}
	}
}

void InitThesisSystemsSingleThreaded(int entityCountPerType)
{
	auto EngineCore = EncosyEngine::GetEncosyCore();
	auto PrimaryWorld = EngineCore->GetPrimaryWorld();
	auto WorldSystemManager = PrimaryWorld->GetWorldSystemManager();
	auto EngineRenderCore = EncosyEngine::GetRenderCore();
	auto MainTextureLoader = EngineRenderCore->GetTextureLoader();
	auto MainMeshLoader = EngineRenderCore->GetMeshLoader();

	int entityModificationsPerFrame = entityCountPerType;


	WorldSystemManager->AddSystem<ThesisModelMatrixBuilderSystemSt>("ThesisModelMatrixBuilderSystemSt");
	WorldSystemManager->AddSystem<ThesisRotationSystemSt>("ThesisRotationSystemSt");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt1>("ThesisEntityModifySystemSt1");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt2>("ThesisEntityModifySystemSt2");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt3>("ThesisEntityModifySystemSt3");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt4>("ThesisEntityModifySystemSt4");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt5>("ThesisEntityModifySystemSt5");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt6>("ThesisEntityModifySystemSt6");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt7>("ThesisEntityModifySystemSt7");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt8>("ThesisEntityModifySystemSt8");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt9>("ThesisEntityModifySystemSt9");
	WorldSystemManager->AddSystem<ThesisEntityModifySystemSt10>("ThesisEntityModifySystemSt10");
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt1>("ThesisEntitySpawningSystemSt1", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt1>("ThesisEntitySpawningSystemSt2", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt2>("ThesisEntitySpawningSystemSt3", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt3>("ThesisEntitySpawningSystemSt4", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt4>("ThesisEntitySpawningSystemSt5", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt5>("ThesisEntitySpawningSystemSt6", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt6>("ThesisEntitySpawningSystemSt7", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt7>("ThesisEntitySpawningSystemSt8", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt8>("ThesisEntitySpawningSystemSt9", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemSt9>("ThesisEntitySpawningSystemSt10", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt1>("ThesisEntityDestroySystemSt1");
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt2>("ThesisEntityDestroySystemSt2");
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt3>("ThesisEntityDestroySystemSt3");
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt4>("ThesisEntityDestroySystemSt4");
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt5>("ThesisEntityDestroySystemSt5");
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt6>("ThesisEntityDestroySystemSt6");
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt7>("ThesisEntityDestroySystemSt7");
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt8>("ThesisEntityDestroySystemSt8");
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt9>("ThesisEntityDestroySystemSt9");
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemSt10>("ThesisEntityDestroySystemSt10");
}

void InitThesisSystemsMultiThreaded(int entityCountPerType, bool allowConcurrent)
{
	auto EngineCore = EncosyEngine::GetEncosyCore();
	auto PrimaryWorld = EngineCore->GetPrimaryWorld();
	auto WorldSystemManager = PrimaryWorld->GetWorldSystemManager();
	auto EngineRenderCore = EncosyEngine::GetRenderCore();
	auto MainTextureLoader = EngineRenderCore->GetTextureLoader();
	auto MainMeshLoader = EngineRenderCore->GetMeshLoader();

	int entityModificationsPerFrame = entityCountPerType;

	
	WorldSystemManager->AddSystem<ThesisModelMatrixBuilderSystemMt>("ThesisModelMatrixBuilderSystemMt", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisRotationSystemMt>("ThesisRotationSystemMt", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt1>("ThesisEntityModifySystemMt1", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt2>("ThesisEntityModifySystemMt2", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt3>("ThesisEntityModifySystemMt3", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt4>("ThesisEntityModifySystemMt4", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt5>("ThesisEntityModifySystemMt5", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt6>("ThesisEntityModifySystemMt6", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt7>("ThesisEntityModifySystemMt7", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt8>("ThesisEntityModifySystemMt8", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt9>("ThesisEntityModifySystemMt9", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityModifySystemMt10>("ThesisEntityModifySystemMt10", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt1>("ThesisEntitySpawningSystemMt1", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt2>("ThesisEntitySpawningSystemMt2", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt3>("ThesisEntitySpawningSystemMt3", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt4>("ThesisEntitySpawningSystemMt4", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt5>("ThesisEntitySpawningSystemMt5", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt6>("ThesisEntitySpawningSystemMt6", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt7>("ThesisEntitySpawningSystemMt7", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt8>("ThesisEntitySpawningSystemMt8", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt9>("ThesisEntitySpawningSystemMt9", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntitySpawningSystemMt10>("ThesisEntitySpawningSystemMt10", entityModificationsPerFrame, MainTextureLoader, MainMeshLoader, allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt1>("ThesisEntityDestroySystemMt1", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt2>("ThesisEntityDestroySystemMt2", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt3>("ThesisEntityDestroySystemMt3", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt4>("ThesisEntityDestroySystemMt4", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt5>("ThesisEntityDestroySystemMt5", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt6>("ThesisEntityDestroySystemMt6", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt7>("ThesisEntityDestroySystemMt7", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt8>("ThesisEntityDestroySystemMt8", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt9>("ThesisEntityDestroySystemMt9", allowConcurrent);
	WorldSystemManager->AddSystem<ThesisEntityDestroySystemMt10>("ThesisEntityDestroySystemMt10", allowConcurrent);
}

export void InitThesisTestSingleThreaded(int entityCountPerType)
{
	InitThesisTestCommon(entityCountPerType);
	InitThesisSystemsSingleThreaded(entityCountPerType);
}

export void InitThesisTestMultiThreaded(int entityCountPerType, bool allowConcurrent)
{
	InitThesisTestCommon(entityCountPerType);
	InitThesisSystemsMultiThreaded(entityCountPerType, allowConcurrent);
}