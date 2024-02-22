module;
#include <fmt/core.h>

export module ECS.EntityManager;


import ECS.ComponentTypeStorage;
import ECS.ComponentManager;
import ECS.Entity;
import ECS.EntityTypeStorage;

import <map>;
import <vector>;
import <string>;
import <memory>;
import <typeindex>;
import <typeinfo>;
import <utility>;
import <algorithm>;
import <format>;
import <span>;
import <unordered_set>;
import <unordered_map>;

export struct EntitySpanFetchInfo
{
	EntityType Type;
	std::string TypeName;
	size_t EntityCount;
	std::vector<EntityStorageLocator> EntityLocators;
};


export struct EntityOperationResult
{
	enum class Result { TypeFound = 0, NotFound, Created, Exists, Error };

	EntityOperationResult()
	{
		OperationResult = Result::NotFound;
		Type = -1;
		TypeName = "DOES NOT EXIST";
	}
	EntityOperationResult(Result message, EntityType entityType, std::string typeName)
	{
		OperationResult = message;
		Type = entityType;
		TypeName = typeName;
	}
	Result OperationResult;
	EntityType Type;
	std::string TypeName;

	std::string GetMessageAsString()
	{
		switch (OperationResult)
		{
		case EntityOperationResult::Result::TypeFound:
			break;
		case EntityOperationResult::Result::NotFound:
			break;
		case EntityOperationResult::Result::Created:
			break;
		case EntityOperationResult::Result::Exists:
			break;
		case EntityOperationResult::Result::Error:
			break;
		default:
			break;
		}
	}
};


export class EntityManager
{
	friend class SystemManager;

public:
	EntityManager(ComponentManager* worldComponentManager) { WorldComponentManager = worldComponentManager; }
	~EntityManager() {}

	template<typename...ComponentType>
	bool ReplaceEntityComponentData(const Entity entity, const ComponentType...Components)
	{
		auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return false;
		}
		auto storageIt = EntityStorages.find(entityType->second);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Entity {} has invalid type in EntityTypeMap: {}", entity, entityType->second);
			return false;
		}
		auto entityStorageID = storageIt->second->GetEntityStorageID(entity);
		(ReplaceEntityComponent(storageIt->second.get(), entityStorageID, Components), ...);
		return true;
	}	

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const std::string entityTypeName, const ComponentTypes...components)
	{
		return CreateEntityWithData(GetEntityTypeInfo(entityTypeName).Type, components...);
	}

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const ComponentTypes...components)
	{
		auto componentTypeIndexes = GatherTypeIndexes<ComponentTypes...>();
		EntityType typeId = FindEntityTypeWithComponents(componentTypeIndexes);

		return CreateEntityWithData(typeId, components...);
	}

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const EntityType entityType, const ComponentTypes...components)
	{
		EntityOperationResult entityOperationResult = GetEntityTypeInfo(entityType);
		if (entityOperationResult.OperationResult == EntityOperationResult::Result::NotFound)
		{
			std::string name = std::format("EntityType{}", EntityTypesCreated);
			entityOperationResult = CreateEntityType<ComponentTypes...>(name);
		}
		auto storage = EntityStorages.find(entityOperationResult.Type)->second.get();
		storage->AddEntityToStorage(EntitiesCreated);
		(CreateEntityComponent(storage, components), ...);

		Entity createdEntity = Entity(EntitiesCreated);
		EntityTypeMap.insert(std::make_pair(EntitiesCreated, entityOperationResult.Type));
		EntitiesCreated++;

		return createdEntity;
	}

	template<typename...ComponentTypes>
	EntityOperationResult CreateEntityType(const std::string entityTypeName)
	{
		auto componentTypeIndexes = GatherTypeIndexes<ComponentTypes...>();
		if (EntityWithComponentsExists(componentTypeIndexes))
		{
			std::string componentNames;
			for (auto name : componentNames)
			{
				componentNames = std::format("{}, {}", componentNames, name);
			}
			return EntityOperationResult(EntityOperationResult::Result::Exists, (EntityType)(-1), std::format("ERROR: EntityType with given components already exists. {}", componentNames));
		}
		if (EntityTypeNameExists(entityTypeName))
		{
			return EntityOperationResult(EntityOperationResult::Result::Exists, (EntityType)(-1), std::format("ERROR: EntityType with name \"{}\" already exists.", entityTypeName));
		}

		fmt::println("Creating new EntityType: {}", entityTypeName);
		auto entityStoragePtr = CreateEntityTypeStorage(entityTypeName);
		(AddComponentToEntityStorage<ComponentTypes>(entityStoragePtr), ...);

		return EntityOperationResult(EntityOperationResult::Result::Created, entityStoragePtr->GetStorageEntityType(), entityTypeName);
	}

	EntityOperationResult GetEntityTypeInfo(const EntityType entityType)
	{
		auto nameIt = EntityTypeNames.find(entityType);
		if (nameIt != EntityTypeNames.end())
		{
			return EntityOperationResult(EntityOperationResult::Result::TypeFound, entityType, nameIt->second);
		}
		
		return EntityOperationResult(EntityOperationResult::Result::NotFound, entityType, std::format("ERROR: Entity with typeID \"{}\" was not found.", entityType));
	}

	EntityOperationResult GetEntityTypeInfo(const std::string entityTypeName)
	{
		auto info = std::find_if(EntityTypeNames.begin(), EntityTypeNames.end(),
			[entityTypeName](const auto& pair) { return pair.second == entityTypeName; });
		if (info != EntityTypeNames.end())
		{
			return EntityOperationResult(EntityOperationResult::Result::TypeFound, info->first, entityTypeName);
		}
		return EntityOperationResult(EntityOperationResult::Result::NotFound, (EntityType)(-1), std::format("ERROR: Entity with typeName \"{}\" was not found.", entityTypeName));
	}

	bool EntityTypeNameExists(const std::string entityTypeName)
	{
		auto info = std::find_if(EntityTypeNames.begin(), EntityTypeNames.end(),
			[entityTypeName](const auto& pair) { return pair.second == entityTypeName; });
		return info != EntityTypeNames.end();
	}

	bool EntityWithComponentsExists(const std::vector<std::type_index>& componentTypeIndexes)
	{
		EntityType typeId = FindEntityTypeWithComponents(componentTypeIndexes);
		return typeId != -1;
	}

	//TODO: This is a brute force way to check compatible entities. Make this smarter in the future.
	std::vector<EntityType> GetEntityTypesWithComponentConditions(
		const std::unordered_set<std::type_index> wantedReadOnlyTypes,
		const std::unordered_set<std::type_index> wantedWriteReadTypes,
		const std::unordered_set<std::type_index> forbiddenTypes)
	{
		std::vector<EntityType> entityTypes;

		if (wantedReadOnlyTypes.size() == 0 && wantedWriteReadTypes.size() == 0)
		{
			return entityTypes;
		}

		for (auto const& pair : EntityStorages)
		{
			auto componentTypes = pair.second->GetEntityComponentTypes();
			bool forbiddenFound = false;
			for (auto const& type : forbiddenTypes)
			{
				forbiddenFound = (std::find(componentTypes.begin(), componentTypes.end(), type) != componentTypes.end());
				if (forbiddenFound) { break; }
			}
			if (forbiddenFound) { continue; }

			bool wantedFound = true;
			for (auto const& type : wantedReadOnlyTypes)
			{
				wantedFound = (std::find(componentTypes.begin(), componentTypes.end(), type) != componentTypes.end());
				if (!wantedFound) { wantedFound = false; break; }
			}
			if (!wantedFound) { continue; }

			wantedFound = true;
			for (auto const& type : wantedWriteReadTypes)
			{
				wantedFound = (std::find(componentTypes.begin(), componentTypes.end(), type) != componentTypes.end());
				if (!wantedFound) { wantedFound = false; break; }
			}
			if (!wantedFound) { continue; }

			entityTypes.push_back(pair.first);
		}
		return entityTypes;
	}

	EntitySpanFetchInfo GetEntityFetchInfo(EntityType typeID)
	{
		auto storage = EntityStorages.find(typeID);
		EntitySpanFetchInfo info = { .Type = typeID, .TypeName = EntityTypeNames.find(typeID)->second, .EntityCount = storage->second->EntityCount, .EntityLocators = storage->second->GetStorageLocators()};
		return info;
	}

	std::vector<EntitySpanFetchInfo> GetEntityFetchInfo(std::vector<EntityType> typeIDs)
	{
		std::vector<EntitySpanFetchInfo> info;

		for (auto id : typeIDs)
		{
			auto storage = EntityStorages.find(id);
			info.push_back({ .Type = id, .TypeName = EntityTypeNames.find(id)->second, .EntityCount = storage->second->EntityCount, .EntityLocators = storage->second->GetStorageLocators() });
		}
		return info;
	}

	template<typename ComponentType>
	std::span<ComponentType const> GetEntityReadOnlyComponentSpan(const Entity entityId)
	{
		auto storage = GetEntityStorage(entityId);
		auto locator = storage->GetComponentStorageLocator<ComponentType>();
		return WorldComponentManager->GetReadOnlyStorageSpan<ComponentType>(locator);
	}

	template<typename ComponentType>
	std::span<ComponentType> GetEntityWriteReadComponentSpan(const Entity entityId)
	{
		auto storage = GetEntityStorage(entityId);
		auto locator = storage->GetComponentStorageLocator<ComponentType>();
		return WorldComponentManager->GetWriteReadStorageSpan<ComponentType>(locator);
	}

	Entity GetEntityIdFromComponentIndex(EntityType entityType, EntityComponentIndex componentIndex)
	{
		auto const& it = EntityStorages.find(entityType);
		return it->second->GetEntityIdFromComponentIndex(componentIndex);
	}

	size_t GetEntitiesCreatedCount()
	{
		return EntitiesCreated;
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromEntity(Entity entity)
	{
		auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return {};
		}
		auto storageIt = EntityStorages.find(entityType->second);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Entity {} has invalid type in EntityTypeMap: {}", entity, entityType->second);
			return {};
		}
		auto storage = storageIt->second.get();
		ComponentStorageLocator locator = storage->GetComponentStorageLocator<ComponentType>;
		EntityComponentIndex index = storage->GetEntityStorageID(entity);
		return WorldComponentManager->GetReadOnlyComponentFromStorage(locator, index);
	}

	EntityComponentIndex GetEntityStorageID(Entity entity)
	{
		auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return {};
		}
		auto storageIt = EntityStorages.find(entityType->second);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Entity {} has invalid type in EntityTypeMap: {}", entity, entityType->second);
			return {};
		}
		auto storage = storageIt->second.get();
		return storage->GetEntityStorageID(entity);
	};

	EntityComponentIndex GetEntityStorageID(Entity entity, EntityType entityType)
	{
		auto storageIt = EntityStorages.find(entityType);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: No storage found for given EntityType", entity, entityType);
			return {};
		}
		auto storage = storageIt->second.get();
		return storage->GetEntityStorageID(entity);
	};

protected:

	EntityTypeStorage* GetEntityStorage(EntityType entityType)
	{
		auto const& it = EntityStorages.find(entityType);
		return it->second.get();
	}

	template<typename ComponentType>
	void CreateEntityComponent(EntityTypeStorage* storage, const ComponentType component)
	{
		auto componentStorageLocator = storage->GetComponentStorageLocator<ComponentType>();
		WorldComponentManager->CreateNewComponentToStorage(componentStorageLocator, component);
	}

	template<typename ComponentType>
	void ReplaceEntityComponent(EntityTypeStorage* storage, const EntityComponentIndex entityStorageID, const ComponentType component)
	{
		auto componentStorageLocator = storage->GetComponentStorageLocator<ComponentType>();
		WorldComponentManager->ReplaceComponentData(componentStorageLocator, entityStorageID);
	}

	template<typename ComponentType>
	void AddComponentToEntityStorage(EntityTypeStorage* entityStoragePtr)
	{
		auto componentStorage = WorldComponentManager->CreateComponentStorage<ComponentType>();
		entityStoragePtr->AddComponentType<ComponentType>(componentStorage);
	}

	EntityTypeStorage* CreateEntityTypeStorage(const std::string entityTypeName)
	{
		EntityType newTypeID = EntityTypesCreated++;
		auto newStorage = std::make_unique<EntityTypeStorage>(newTypeID);
		EntityTypeNames.insert(std::pair(newTypeID, entityTypeName));
		auto storageIt = EntityStorages.insert(std::pair(newTypeID, std::move(newStorage)));

		return storageIt.first->second.get();
	}

	template<typename ComponentType>
	void AddTypeIndexToVector(std::vector<std::type_index>& indexVector)
	{
		indexVector.push_back(typeid(ComponentType));
	}

	template<typename...ComponentTypes>
	std::vector<std::type_index> GatherTypeIndexes()
	{
		std::vector<std::type_index> componentTypeIndexes;
		(AddTypeIndexToVector<ComponentTypes>(componentTypeIndexes), ...);
		std::sort(componentTypeIndexes.begin(), componentTypeIndexes.end());
		return componentTypeIndexes;
	}

	EntityType FindEntityTypeWithComponents(const std::vector<std::type_index> &indexVector)
	{
		auto foundStorage = std::find_if(EntityStorages.begin(), EntityStorages.end(),
			[&indexVector](const auto& pair) { return pair.second->GetEntityComponentTypes() == indexVector; });
		if (foundStorage == EntityStorages.end())
		{
			return -1;
		}
		return foundStorage->first;
	}

private:
	
	std::unordered_map<Entity, EntityType> EntityTypeMap;
	std::map<EntityType, std::string> EntityTypeNames;
	std::map<EntityType, std::unique_ptr<EntityTypeStorage>> EntityStorages;
	ComponentManager* WorldComponentManager;
	size_t EntitiesCreated = 0;
	size_t EntityTypesCreated = 0;
};
