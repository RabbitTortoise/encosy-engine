module;
export module ECS.EntityManager;


import ECS.ComponentTypeStorage;
import ECS.ComponentManager;
import ECS.Entity;
import ECS.EntityTypeStorage;

import <map>;
import <vector>;
import <string>;
import <memory>;
import <iostream>;
import <typeindex>;
import <typeinfo>;
import <utility>;
import <algorithm>;
import <format>;
import <span>;
import <unordered_set>;

export struct EntitySpanFetchInfo
{
	EntityTypeID TypeID;
	size_t EntityCount;
	std::vector<EntityStorageLocator> EntityLocators;
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
		auto storageIt = EntityStorages.find(entity.GetType());
		if (storageIt == EntityStorages.end())
		{
			return false;
		}
		auto entityStorageID = storageIt->second->GetEntityStorageID(entity.GetID());
		(ReplaceEntityComponent(storageIt->second.get(), entityStorageID, Components), ...);
		return true;
	}	

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const std::string entityTypeName, const ComponentTypes...components)
	{
		return CreateEntityWithData(GetEntityTypeInfo(entityTypeName).TypeID, components...);
	}

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const ComponentTypes...components)
	{
		auto componentTypeIndexes = GatherTypeIndexes<ComponentTypes...>();
		EntityTypeID typeId = FindEntityTypeWithComponents(componentTypeIndexes);

		return CreateEntityWithData(typeId, components...);
	}

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const EntityTypeID entityType, const ComponentTypes...components)
	{
		auto entityTypeInfo = GetEntityTypeInfo(entityType);
		if (entityTypeInfo.Message == EntityTypeInfo::Message::NotFound)
		{
			std::string name = std::format("EntityType{}", EntityTypesCreated);
			std::cout << "Creating: " << name << std::endl;
			entityTypeInfo = CreateEntityType<ComponentTypes...>(name);
		}
		auto storage = EntityStorages.find(entityTypeInfo.TypeID)->second.get();
		storage->AddEntityToStorage(EntitiesCreated);
		(CreateEntityComponent(storage, components), ...);

		Entity createdEntity = Entity(EntitiesCreated, entityTypeInfo.TypeID);
		EntitiesCreated++;

		return createdEntity;
	}

	template<typename...ComponentTypes>
	EntityTypeInfo CreateEntityType(const std::string entityTypeName)
	{
		auto componentTypeIndexes = GatherTypeIndexes<ComponentTypes...>();
		if (EntityWithComponentsExists(componentTypeIndexes))
		{
			std::string componentNames;
			for (auto name : componentNames)
			{
				componentNames = std::format("{}, {}", componentNames, name);
			}
			return EntityTypeInfo(EntityTypeInfo::Message::Exists, (EntityTypeID)(-1), std::format("ERROR: EntityType with given components already exists. {}", componentNames));
		}
		if (EntityTypeNameExists(entityTypeName))
		{
			return EntityTypeInfo(EntityTypeInfo::Message::Exists, (EntityTypeID)(-1), std::format("ERROR: EntityType with name \"{}\" already exists.", entityTypeName));
		}

		auto entityStoragePtr = CreateEntityTypeStorage(entityTypeName);
		(AddComponentToEntityStorage<ComponentTypes>(entityStoragePtr), ...);

		return EntityTypeInfo(EntityTypeInfo::Message::Created, entityStoragePtr->GetStorageEntityTypeID(), entityTypeName);
	}

	EntityTypeInfo GetEntityTypeInfo(const EntityTypeID entityTypeId)
	{
		auto nameIt = EntityTypeNames.find(entityTypeId);
		if (nameIt != EntityTypeNames.end())
		{
			return EntityTypeInfo(EntityTypeInfo::Message::TypeFound, entityTypeId, nameIt->second);
		}
		
		return EntityTypeInfo(EntityTypeInfo::Message::NotFound, entityTypeId, std::format("ERROR: Entity with typeID \"{}\" was not found.", entityTypeId));
	}

	EntityTypeInfo GetEntityTypeInfo(const std::string entityTypeName)
	{
		auto info = std::find_if(EntityTypeNames.begin(), EntityTypeNames.end(),
			[entityTypeName](const auto& pair) { return pair.second == entityTypeName; });
		if (info != EntityTypeNames.end())
		{
			return EntityTypeInfo(EntityTypeInfo::Message::TypeFound, info->first, entityTypeName);
		}
		return EntityTypeInfo(EntityTypeInfo::Message::NotFound, (EntityTypeID)(-1), std::format("ERROR: Entity with typeName \"{}\" was not found.", entityTypeName));
	}

	bool EntityTypeNameExists(const std::string entityTypeName)
	{
		auto info = std::find_if(EntityTypeNames.begin(), EntityTypeNames.end(),
			[entityTypeName](const auto& pair) { return pair.second == entityTypeName; });
		return info != EntityTypeNames.end();
	}

	bool EntityWithComponentsExists(const std::vector<std::type_index>& componentTypeIndexes)
	{
		EntityTypeID typeId = FindEntityTypeWithComponents(componentTypeIndexes);
		return typeId != -1;
	}

	//TODO: This is a brute force way to check compatible entities. Make this smarter in the future.
	std::vector<EntityTypeID> GetEntityTypesWithComponentConditions(
		const std::unordered_set<std::type_index> wantedReadOnlyTypes,
		const std::unordered_set<std::type_index> wantedWriteReadTypes,
		const std::unordered_set<std::type_index> forbiddenTypes)
	{
		std::vector<EntityTypeID> entityTypes;
		for (auto const& pair : EntityStorages)
		{
			auto componentTypes = pair.second->GetEntityComponentTypes();
			bool forbiddenFound = false;
			for (auto const& type : forbiddenTypes)
			{
				bool forbiddenFound = (std::find(componentTypes.begin(), componentTypes.end(), type) != componentTypes.end());
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

	EntitySpanFetchInfo GetEntityFetchInfo(EntityTypeID typeID)
	{
		auto storage = EntityStorages.find(typeID);
		EntitySpanFetchInfo info = { .TypeID = typeID, .EntityCount = storage->second->EntityCount, .EntityLocators = storage->second->GetStorageLocators() };
		return info;
	}

	std::vector<EntitySpanFetchInfo> GetEntityFetchInfo(std::vector<EntityTypeID> typeIDs)
	{
		std::vector<EntitySpanFetchInfo> info;

		for (auto id : typeIDs)
		{
			auto storage = EntityStorages.find(id);
			info.push_back({ .TypeID = id, .EntityCount = storage->second->EntityCount, .EntityLocators = storage->second->GetStorageLocators() });
		}
		return info;
	}

	template<typename ComponentType>
	std::span<ComponentType const> GetEntityReadOnlyComponentSpan(const EntityID entityId)
	{
		auto storage = GetEntityStorage(entityId);
		auto locator = storage->GetComponentStorageLocator<ComponentType>();
		return WorldComponentManager->GetReadOnlyStorageSpan<ComponentType>(locator);
	}

	template<typename ComponentType>
	std::span<ComponentType> GetEntityWriteReadComponentSpan(const EntityID entityId)
	{
		auto storage = GetEntityStorage(entityId);
		auto locator = storage->GetComponentStorageLocator<ComponentType>();
		return WorldComponentManager->GetWriteReadStorageSpan<ComponentType>(locator);
	}

	EntityID GetEntityIdFromComponentIndex(EntityTypeID entityTypeId, EntityComponentIndex componentIndex)
	{
		auto const& it = EntityStorages.find(entityTypeId);
		return it->second->GetEntityIdFromComponentIndex(componentIndex);
	}

protected:

	EntityTypeStorage* GetEntityStorage(EntityTypeID entityTypeId)
	{
		auto const& it = EntityStorages.find(entityTypeId);
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
		EntityTypeID newTypeID = EntityTypesCreated++;
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

	EntityTypeID FindEntityTypeWithComponents(const std::vector<std::type_index> &indexVector)
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
	
	std::map<EntityTypeID, std::string> EntityTypeNames;
	std::map<EntityTypeID, std::unique_ptr<EntityTypeStorage>> EntityStorages;
	ComponentManager* WorldComponentManager;
	size_t EntitiesCreated = 0;
	size_t EntityTypesCreated = 0;
};
