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
	EntityOperationResult(const Result result, const EntityType entityType, const std::string& typeName)
	{
		OperationResult = result;
		Type = entityType;
		TypeName = typeName;
	}
	Result OperationResult;
	EntityType Type;
	std::string TypeName;

	constexpr std::string GetMessageAsString() const
	{
		switch (OperationResult)
		{
		case Result::TypeFound:
			return "TypeFound";
		case Result::NotFound:
			return "NotFound";
		case Result::Created:
			return "Created";
		case Result::Exists:
			return "Exists";
		case Result::Error:
			return "Error";
		default:
			return "";
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
		const auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return false;
		}
		const auto storageIt = EntityStorages.find(entityType->second);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Entity {} has invalid type in EntityTypeMap: {}", entity, entityType->second);
			return false;
		}
		auto entityComponentIndex = storageIt->second->GetEntityComponentIndex(entity);
		(ReplaceEntityComponent(storageIt->second.get(), entityComponentIndex, Components), ...);
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
			const std::string name = std::format("EntityType{}", EntityTypesCreated);
			entityOperationResult = CreateEntityType<ComponentTypes...>(name);
		}
		auto storage = EntityStorages.find(entityOperationResult.Type)->second.get();
		storage->AddEntityToStorage(EntitiesCreated);
		(CreateEntityComponent(storage, components), ...);

		const Entity createdEntity = EntitiesCreated;
		EntityTypeMap.insert(std::make_pair(EntitiesCreated, entityOperationResult.Type));
		EntitiesCreated++;
		CurrentEntityCount++;

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
			return EntityOperationResult(EntityOperationResult::Result::Exists, (-1), std::format("ERROR: EntityType with given components already exists. {}", componentNames));
		}
		if (EntityTypeNameExists(entityTypeName))
		{
			return EntityOperationResult(EntityOperationResult::Result::Exists, (-1), std::format("ERROR: EntityType with name \"{}\" already exists.", entityTypeName));
		}

		const auto entityStoragePtr = CreateEntityTypeStorage(entityTypeName);
		(AddComponentToEntityStorage<ComponentTypes>(entityStoragePtr), ...);

		return EntityOperationResult(EntityOperationResult::Result::Created, entityStoragePtr->GetStorageEntityType(), entityTypeName);
	}

	EntityOperationResult GetEntityTypeInfo(const EntityType entityType)
	{
		if (EntityTypeNames.find(entityType) != EntityTypeNames.end())
		{
			return EntityOperationResult(EntityOperationResult::Result::TypeFound, entityType, EntityTypeNames.find(entityType)->second);
		}
		
		return EntityOperationResult(EntityOperationResult::Result::NotFound, entityType, std::format("ERROR: Entity with typeID \"{}\" was not found.", entityType));
	}

	EntityOperationResult GetEntityTypeInfo(const std::string& entityTypeName)
	{
		const auto info = std::ranges::find_if(EntityTypeNames,[entityTypeName](const auto& pair)
		{
			return pair.second == entityTypeName;
		});
		if (info != EntityTypeNames.end())
		{
			return EntityOperationResult(EntityOperationResult::Result::TypeFound, info->first, entityTypeName);
		}
		return EntityOperationResult(EntityOperationResult::Result::NotFound, (-1), std::format("ERROR: Entity with typeName \"{}\" was not found.", entityTypeName));
	}

	bool EntityTypeNameExists(const std::string& entityTypeName)
	{
		const auto info = std::ranges::find_if(EntityTypeNames,[entityTypeName](const auto& pair)
		{
			return pair.second == entityTypeName;
		});
		return info != EntityTypeNames.end();
	}

	bool EntityWithComponentsExists(const std::vector<std::type_index>& componentTypeIndexes)
	{
		const EntityType typeId = FindEntityTypeWithComponents(componentTypeIndexes);
		return typeId != -1;
	}

	//TODO: This is a brute force way to check compatible entities. This could be optimized by only updating this when a new type of entity is added.
	std::vector<EntityType> GetEntityTypesWithComponentConditions(
		const std::unordered_set<std::type_index>& wantedReadOnlyTypes,
		const std::unordered_set<std::type_index>& wantedWriteReadTypes,
		const std::unordered_set<std::type_index>& forbiddenTypes)
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
				forbiddenFound = (std::ranges::find(componentTypes, type) != componentTypes.end());
				if (forbiddenFound) { break; }
			}
			if (forbiddenFound) { continue; }

			bool wantedFound = true;
			for (auto const& type : wantedReadOnlyTypes)
			{
				wantedFound = (std::ranges::find(componentTypes, type) != componentTypes.end());
				if (!wantedFound) { wantedFound = false; break; }
			}
			if (!wantedFound) { continue; }

			wantedFound = true;
			for (auto const& type : wantedWriteReadTypes)
			{
				wantedFound = (std::ranges::find(componentTypes, type) != componentTypes.end());
				if (!wantedFound) { wantedFound = false; break; }
			}
			if (!wantedFound) { continue; }

			entityTypes.push_back(pair.first);
		}
		return entityTypes;
	}

	EntitySpanFetchInfo GetEntityFetchInfo(EntityType typeID)
	{
		const auto storage = EntityStorages.find(typeID);
		EntitySpanFetchInfo info = { .Type = typeID, .TypeName = EntityTypeNames.find(typeID)->second, .EntityCount = storage->second->GetEntityCount(), .EntityLocators = storage->second->GetEntityStorageLocators()};
		return info;
	}

	std::vector<EntitySpanFetchInfo> GetEntityFetchInfo(const std::vector<EntityType>& typeIDs)
	{
		std::vector<EntitySpanFetchInfo> info;

		for (auto id : typeIDs)
		{
			const auto storage = EntityStorages.find(id);
			info.push_back({ .Type = id, .TypeName = EntityTypeNames.find(id)->second, .EntityCount = storage->second->GetEntityCount(), .EntityLocators = storage->second->GetEntityStorageLocators() });
		}
		return info;
	}

	template<typename ComponentType>
	std::span<ComponentType const> GetEntityReadOnlyComponentSpan(const EntityType entityType)
	{
		const auto storage = GetStorageByEntityType(entityType);
		auto locator = storage->GetComponentStorageLocator<ComponentType>();
		return WorldComponentManager->GetReadOnlyStorageSpan<ComponentType>(locator);
	}

	template<typename ComponentType>
	std::span<ComponentType> GetEntityWriteReadComponentSpan(const EntityType entityType)
	{
		const auto storage = GetStorageByEntityType(entityType);
		auto locator = storage->GetComponentStorageLocator<ComponentType>();
		return WorldComponentManager->GetWriteReadStorageSpan<ComponentType>(locator);
	}

	Entity GetEntityIdFromComponentIndex(EntityType entityType, EntityComponentIndex componentIndex)
	{
		auto const& it = EntityStorages.find(entityType);
		return it->second->GetEntityIdFromComponentIndex(componentIndex);
	}

	size_t GetCurrentEntityCount() const
	{
		return CurrentEntityCount;
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromEntity(Entity entity, EntityType entityType)
	{
		const auto storageIt = EntityStorages.find(entityType);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Can't find type in EntityTypeMap: {}", entity, entityType);
			return {};
		}
		const auto storage = storageIt->second.get();
		const ComponentStorageLocator locator = storage->GetComponentStorageLocator<ComponentType>();
		const EntityComponentIndex index = storage->GetEntityComponentIndex(entity);
		return WorldComponentManager->GetReadOnlyComponentFromStorage<ComponentType>(locator, index);
	}


	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromEntity(Entity entity)
	{
		const auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return {};
		}
		return GetReadOnlyComponentFromEntity<ComponentType>(entity, entityType->second);
	}

	template<typename ComponentType>
	ComponentType* GetWriteReadComponentFromEntity(Entity entity, EntityType entityType)
	{
		const auto storageIt = EntityStorages.find(entityType);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Can't find type in EntityTypeMap: {}", entity, entityType);
			return {};
		}
		const auto storage = storageIt->second.get();
		const ComponentStorageLocator locator = storage->GetComponentStorageLocator<ComponentType>();
		const EntityComponentIndex index = storage->GetEntityComponentIndex(entity);
		return WorldComponentManager->GetWriteReadComponentFromStorage<ComponentType>(locator, index);
	}

	template<typename ComponentType>
	ComponentType* GetWriteReadComponentFromEntity(Entity entity)
	{
		const auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return {};
		}
		return GetWriteReadComponentFromEntity<ComponentType>(entity, entityType->second);
	}

	EntityComponentIndex GetEntityComponentIndex(Entity entity)
	{
		const auto storage = GetStorageByEntity(entity);
		return storage->GetEntityComponentIndex(entity);
	};

	EntityComponentIndex GetEntityComponentIndex(Entity entity, EntityType entityType)
	{
		const auto storageIt = EntityStorages.find(entityType);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: No storage found for given EntityType", entity, entityType);
			return {};
		}
		const auto storage = storageIt->second.get();
		return storage->GetEntityComponentIndex(entity);
	};

	bool DeleteEntity(Entity entity)
	{
		const auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return false;
		}

		const auto storageIt = EntityStorages.find(entityType->second);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: No storage found for given EntityType", entity, entityType->second);
			return false;
		}
		const auto storage = storageIt->second.get();
		const auto entityComponentIndex = storage->GetEntityComponentIndex(entity);
		const auto componentStorageLocators = storage->GetComponentStorageLocators();
		if(storage->RemoveEntity(entity))
		{
			for (const auto& locator : componentStorageLocators)
			{
				WorldComponentManager->RemoveComponent(locator, entityComponentIndex);
			}

			EntityTypeMap.erase(entityType);
			CurrentEntityCount--;
			return true;
		}
		return false;
	}

	template<typename ComponentType>
	bool HasComponentType(const Entity entity) const
	{
		const auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return false;
		}

		const auto storageIt = EntityStorages.find(entityType->second);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: No storage found for given EntityType", entity, entityType->second);
			return false;
		}
		const auto storage = storageIt->second.get();
		return storage->HasComponentType<ComponentType>();
	}

	template<typename... ComponentTypes>
	void ModifyEntityComponents(const Entity entity)
	{
		const EntityType entityType = GetEntityType(entity);
		ModifyEntityComponents<ComponentTypes...>(entity, entityType);
	}

	template<typename... ComponentTypes>
	bool ModifyEntityComponents(const Entity entity, const EntityType entityType)
	{
		auto oldStorage = GetStorageByEntityType(entityType);
		auto entityLocator = oldStorage->GetEntityStorageLocator(entity);
		if(entityLocator.Entity == -1)
		{
			return false;
		}

		auto componentStorageLocators = oldStorage->GetComponentStorageLocators();
		auto componentTypes = oldStorage->GetEntityComponentTypes();
		auto newStorageTypes = componentTypes;

		auto deleteTypes = std::vector<std::type_index>();
		auto newTypes = std::vector<std::type_index>();
		auto modifyComponentTypes = GatherTypeIndexes<ComponentTypes...>();
		for (auto type : modifyComponentTypes)
		{
			auto it = std::ranges::find(componentTypes, type);
			if (it == componentTypes.end())
			{
				newTypes.push_back(type);
				newStorageTypes.push_back(type);
			}
			else
			{
				deleteTypes.push_back(type);
				auto it = std::ranges::find(newStorageTypes, type);
				newStorageTypes.erase(it);
			}
		}
		auto copyLocators = componentStorageLocators;
		for (auto type : modifyComponentTypes)
		{
			auto it = std::ranges::find(copyLocators, type, &ComponentStorageLocator::ComponentType);
			if (it != copyLocators.end())
			{
				copyLocators.erase(it);
			}
		}

		std::vector<ComponentStorageLocator> newCopyStorageLocators;
		std::vector<ComponentStorageLocator> newCreatedStorageLocators;

		std::ranges::sort(newStorageTypes, {});

		// Check if EntityStorage with the components exists
		auto newStorage = FindEntityStorageWithComponents(newStorageTypes);
		if (!newStorage)
		{
			std::string newName = std::format("EntityType{}", EntityTypesCreated);
			// Create Entity Storage.
			newStorage = CreateEntityTypeStorage(newName);
			// Create new component storages
			for (auto locator : copyLocators)
			{
				auto newlocator = WorldComponentManager->CreateSimilarStorage(locator);
				newCopyStorageLocators.push_back(newlocator);
				newStorage->AddComponentType(newlocator);
			}
			(CreateNewStorages<ComponentTypes>(newCreatedStorageLocators, newTypes), ...);
			for (auto locator : newCreatedStorageLocators)
			{
				newStorage->AddComponentType(locator);
			}
		}
		else
		{
			auto storageLocators = newStorage->GetComponentStorageLocators();
			for(const auto& locator : storageLocators)
			{
				auto it = std::ranges::find(copyLocators, locator.ComponentType, &ComponentStorageLocator::ComponentType);
				if(it != copyLocators.end())
				{
					newCopyStorageLocators.push_back(locator);
				}
				else
				{
					newCreatedStorageLocators.push_back(locator);
				}
			}
		}

		//Add the entity to new storage
		newStorage->AddEntityToStorage(entity);
		// Copy all components to new storage.
		for (const auto& destination : newCopyStorageLocators)
		{
			auto origin = *std::ranges::find(copyLocators, destination.ComponentType, &ComponentStorageLocator::ComponentType);
			if(!WorldComponentManager->CopyComponentToOtherStorage(origin, destination, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component copy to new storage failed!");
			};
			if(!WorldComponentManager->RemoveComponent(origin, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component removing from old storage failed!");
			};
		}
		for (const auto& destination : newCreatedStorageLocators)
		{
			WorldComponentManager->CreateNewComponentToStorage(destination);
		}

		// Delete entity from old storages.
		oldStorage->RemoveEntity(entity);

		auto it = EntityTypeMap.find(entity);
		it->second = newStorage->GetStorageEntityType();

		return true;
	}



protected:


	template<typename ComponentType>
	void CreateNewStorages(std::vector<ComponentStorageLocator>& newStorageLocators, const std::vector<std::type_index>& newTypes)
	{
		const std::type_index componentType = typeid(ComponentType);
		if(std::ranges::find(newTypes, componentType) != newTypes.end())
		{
			auto locator = WorldComponentManager->CreateComponentStorage<ComponentType>();
			newStorageLocators.push_back(locator);
		}
	}

	EntityTypeStorage* GetStorageByEntity(Entity entity)
	{
		const auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return {};
		}
		const auto storageIt = EntityStorages.find(entityType->second);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Entity {} has invalid type in EntityTypeMap: {}", entity, entityType->second);
			return {};
		}
		return storageIt->second.get();
	}

	EntityTypeStorage* GetStorageByEntityType(EntityType entityType)
	{
		auto const& storageIt = EntityStorages.find(entityType);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Type not found from EntityTypeMap: {}", entityType);
			return {};
		}
		return storageIt->second.get();
	}

	template<typename ComponentType>
	void CreateEntityComponent(EntityTypeStorage* storage, const ComponentType component)
	{
		auto componentStorageLocator = storage->GetComponentStorageLocator<ComponentType>();
		WorldComponentManager->CreateNewComponentToStorage(componentStorageLocator, component);
	}

	template<typename ComponentType>
	void ReplaceEntityComponent(EntityTypeStorage* storage, const EntityComponentIndex entityComponentIndex, const ComponentType component)
	{
		auto componentStorageLocator = storage->GetComponentStorageLocator<ComponentType>();
		WorldComponentManager->ReplaceComponentData(componentStorageLocator, entityComponentIndex);
	}

	template<typename ComponentType>
	void AddComponentToEntityStorage(EntityTypeStorage* entityStoragePtr)
	{
		auto componentStorage = WorldComponentManager->CreateComponentStorage<ComponentType>();
		entityStoragePtr->AddComponentType(componentStorage);
	}

	EntityTypeStorage* CreateEntityTypeStorage(const std::string& entityTypeName)
	{
		fmt::println("Creating new EntityType with name: {}", entityTypeName);

		EntityType newTypeID = EntityTypesCreated++;
		auto newStorage = std::make_unique<EntityTypeStorage>(newTypeID);
		EntityTypeNames.insert(std::pair(newTypeID, entityTypeName));
		const auto storageIt = EntityStorages.insert(std::pair(newTypeID, std::move(newStorage)));

		return storageIt.first->second.get();
	}

	EntityType GetEntityType(const Entity entity)
	{
		const auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return -1;
		}
		return entityType->second;
	}


	template<typename ComponentType>
	static void AddTypeIndexToVector(std::vector<std::type_index>& indexVector)
	{
		indexVector.push_back(typeid(ComponentType));
	}

	template<typename...ComponentTypes>
	static std::vector<std::type_index> GatherTypeIndexes()
	{
		std::vector<std::type_index> componentTypeIndexes;
		(AddTypeIndexToVector<ComponentTypes>(componentTypeIndexes), ...);
		std::ranges::sort(componentTypeIndexes, {});
		return componentTypeIndexes;
	}

	EntityType FindEntityTypeWithComponents(const std::vector<std::type_index> &indexVector)
	{
		const auto foundStorage = std::ranges::find_if(EntityStorages,[&indexVector](const auto& pair)
		{
			return pair.second->GetEntityComponentTypes() == indexVector;
		});
		if (foundStorage == EntityStorages.end())
		{
			return -1;
		}
		return foundStorage->first;
	}

	EntityTypeStorage* FindEntityStorageWithComponents(const std::vector<std::type_index>& indexVector)
	{
		const auto foundStorage = std::ranges::find_if(EntityStorages,[&indexVector](const auto& pair)
		{
			return pair.second->GetEntityComponentTypes() == indexVector;
		});
		if (foundStorage == EntityStorages.end())
		{
			return nullptr;
		}
		return foundStorage->second.get();
	}

private:

	//std::unordered_map<Entity, EntityType> EntityTypeMap;
	std::map<Entity, EntityType> EntityTypeMap;
	std::map<EntityType, std::string> EntityTypeNames;
	std::map<EntityType, std::unique_ptr<EntityTypeStorage>> EntityStorages;

	ComponentManager* WorldComponentManager;
	size_t EntitiesCreated = 0;
	size_t CurrentEntityCount = 0;
	size_t EntityTypesCreated = 0;
};
