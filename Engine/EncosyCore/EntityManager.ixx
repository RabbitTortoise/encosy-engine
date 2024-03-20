module;
#include <fmt/core.h>

export module EncosyCore.EntityManager;

import EncosyCore.SharedBetweenManagers;
import EncosyCore.ComponentTypeStorage;
import EncosyCore.ComponentManager;
import EncosyCore.Entity;
import EncosyCore.EntityTypeStorage;

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
import <mutex>;

export struct EntitySpanFetchInfo
{
	EntityType Type;
	std::string TypeName;
	size_t EntityCount;
	std::span<EntityStorageLocator const> EntityLocators;
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
	EntityManager(SharedBetweenManagers* sharedBetweenManagers, ComponentManager* worldComponentManager)
	{
		WorldSharedBetweenManagers = sharedBetweenManagers;
		WorldComponentManager = worldComponentManager;
	}
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
	Entity CreateEntityWithDataThreaded(const EntityType entityType, const std::unordered_set<EntityType>& access, const ComponentTypes...components)
	{

		if (!access.contains(entityType))
		{
			fmt::println("ERROR: CreateEntityWithDataThreaded(): Passed access set for entity creation did not contain the correct entity type, required: {}", entityType);
			return false;
		}

		EntityOperationResult entityOperationResult = GetEntityTypeInfo(entityType);

		std::scoped_lock lock(DestructiveEditMutex);

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

	std::vector<EntityType> GetEntityTypesWithComponent(const std::type_index componentType) const
	{
		std::vector<EntityType> entityTypes;

		for (auto const& pair : EntityStorages)
		{
			auto componentTypes = pair.second->GetEntityComponentTypes();
			if(std::ranges::find(componentTypes, componentType) != componentTypes.end())
			{
				entityTypes.push_back(pair.first);
			}
		}
		return entityTypes;
	}

	EntitySpanFetchInfo GetEntityFetchInfo(const EntityType typeId) const
	{
		const auto storage = EntityStorages.find(typeId);
		EntitySpanFetchInfo info = { .Type = typeId, .TypeName = EntityTypeNames.find(typeId)->second, .EntityCount = storage->second->GetEntityCount(), .EntityLocators = storage->second->GetEntityStorageLocators()};
		return info;
	}

	std::vector<EntitySpanFetchInfo> GetEntityFetchInfo(const std::vector<EntityType>& typeIds) const
	{
		std::vector<EntitySpanFetchInfo> info;
		info.reserve(typeIds.size());

		for (auto id : typeIds)
		{
			const auto storage = EntityStorages.find(id);
			EntitySpanFetchInfo i = { .Type = id, .TypeName = EntityTypeNames.find(id)->second, .EntityCount = storage->second->GetEntityCount(), .EntityLocators = storage->second->GetEntityStorageLocators() };
			info.emplace_back(i);
		}
		return info;
	}

	template<typename ComponentType>
	std::span<ComponentType const> GetEntityReadOnlyComponentSpan(const EntityType entityType)
	{
		const auto storage = GetStorageByEntityType(entityType);
		auto componentStorage = storage->GetComponentStoragePointer<ComponentType>();

		return WorldComponentManager->GetReadOnlyStorageSpan<ComponentType>(componentStorage);
	}

	template<typename ComponentType>
	std::span<ComponentType> GetEntityWriteReadComponentSpan(const EntityType entityType)
	{
		const auto storage = GetStorageByEntityType(entityType);
		auto componentStorage = storage->GetComponentStoragePointer<ComponentType>();
		return WorldComponentManager->GetWriteReadStorageSpan<ComponentType>(componentStorage);
	}

	Entity GetEntityFromComponentIndex(EntityType entityType, EntityComponentIndex componentIndex)
	{
		auto const& it = EntityStorages.find(entityType);
		return it->second->GetEntityFromComponentIndex(componentIndex);
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
		auto componentStorage = storage->GetComponentStoragePointer<ComponentType>();
		const EntityComponentIndex index = storage->GetEntityComponentIndex(entity);
		return WorldComponentManager->GetReadOnlyComponentFromStorage<ComponentType>(componentStorage, index);
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
	ComponentType GetReadOnlyComponentFromEntityThreaded(Entity entity)
	{
		std::scoped_lock lock(DestructiveEditMutex);
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
		auto componentStorage = storage->GetComponentStoragePointer<ComponentType>();
		const EntityComponentIndex index = storage->GetEntityComponentIndex(entity);
		return WorldComponentManager->GetWriteReadComponentFromStorage<ComponentType>(componentStorage, index);
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

	typedef std::_List_iterator<std::_List_val<std::_List_simple_types<std::pair<const unsigned long long, unsigned long long>>>> EntityTypeMapIt;

	bool DeleteEntityThreaded(const Entity entity, const std::unordered_set<EntityType>& access)
	{
		std::scoped_lock lock(DestructiveEditMutex);

		const auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return false;
		}
		if (!access.contains(entityType->second))
		{
			fmt::println("ERROR: DeleteEntityThreaded(): Passed access set for entity {} did not contain the correct entity type, required: {}", entity, entityType->second);
			return false;
		}
		return DeleteEntity(entity, entityType);
	}

	bool DeleteEntity(const Entity entity)
	{
		const auto entityType = EntityTypeMap.find(entity);
		if (entityType == EntityTypeMap.end())
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return false;
		}
		
		return DeleteEntity(entity, entityType);
	}

	bool DeleteEntity(const Entity entity, const EntityTypeMapIt& entityType)
	{
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
				auto componentStorage = storage->GetComponentStoragePointer(locator.ComponentType);
				WorldComponentManager->RemoveComponent(componentStorage, entityComponentIndex);
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

	template<typename ComponentType>
	bool HasComponentTypeThreaded(const Entity entity)
	{
		std::scoped_lock lock(DestructiveEditMutex);

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
	EntityType ModifyEntityComponents(const Entity entity)
	{
		const EntityType entityType = GetEntityType(entity);
		return ModifyEntityComponents<ComponentTypes...>(entity, entityType);
	}

	template<typename... DeleteComponentTypes, typename... NewComponentTypes>
	EntityType ModifyEntityComponentsThreaded(const Entity entity, const EntityType entityType, const std::unordered_set<EntityType>& access, NewComponentTypes... newComponents)
	{
		if (!access.contains(entityType))
		{
			fmt::println("ERROR: ModifyEntityComponentsThreaded(): Passed access set for entity {} did not contain the original entity type, required: {}", entity, entityType);
			return false;
		}

		std::scoped_lock lock(DestructiveEditMutex);

		auto oldStorage = GetStorageByEntityType(entityType);
		auto entityLocator = oldStorage->GetEntityStorageLocator(entity);
		if (entityLocator.Entity == -1)
		{
			return false;
		}
		auto locatorSpan = oldStorage->GetComponentStorageLocators();
		std::vector<ComponentStorageLocator> componentStorageLocators;
		componentStorageLocators.reserve(locatorSpan.size());
		std::ranges::copy(locatorSpan, std::back_inserter(componentStorageLocators));

		std::vector<std::type_index> deleteTypes = GatherTypeIndexes<DeleteComponentTypes...>();
		std::vector<std::type_index> replacedTypes;
		std::vector<std::type_index> newTypes = GatherTypeIndexes<NewComponentTypes...>();
		std::vector<std::type_index> newStorageTypes = newTypes;

		// Remove deleted components from copy list.
		auto copyLocators = componentStorageLocators;
		for (auto type : deleteTypes)
		{
			auto it = std::ranges::find(copyLocators, type, &ComponentStorageLocator::ComponentType);
			copyLocators.erase(it);
		}
		// Gather component types that are replaced
		for (auto locator : copyLocators)
		{
			auto it = std::ranges::find(newTypes, locator.ComponentType);
			if (it != newTypes.end())
			{
				replacedTypes.push_back(*it);
			}
		}
		// Gather component types that are copied
		for (auto type : newTypes)
		{
			auto it = std::ranges::find(copyLocators, type, &ComponentStorageLocator::ComponentType);
			if (it != copyLocators.end())
			{
				copyLocators.erase(it);
			}
		}
		// Gather component types the new storage will have
		for (auto locator : copyLocators)
		{
			if (std::ranges::find(newStorageTypes, locator.ComponentType) == newStorageTypes.end())
			{
				newStorageTypes.push_back(locator.ComponentType);
			}
		}

		std::vector<ComponentStorageLocator> newCopyStorageLocators;
		std::vector<ComponentStorageLocator> newCreatedStorageLocators;

		std::ranges::sort(newStorageTypes, {});

		auto newStorage = FindEntityStorageWithComponents(newStorageTypes);

		if(newStorage == nullptr)
		{
			fmt::println("ERROR: ModifyEntityComponentsThreaded(): The new EntityType must exist when using the threaded version of this function!");
			abort();
		}
		auto newEntityType = newStorage->GetStorageEntityType();
		if (!access.contains(newEntityType))
		{
			fmt::println("ERROR: ModifyEntityComponentsThreaded(): Passed access set for entity {} did not contain the modified entity type, required: {}", entity, newEntityType);
			abort();
		}

		auto newStorageLocators = newStorage->GetComponentStorageLocators();
		for (const auto& locator : newStorageLocators)
		{
			if (std::ranges::find(copyLocators, locator.ComponentType, &ComponentStorageLocator::ComponentType) != copyLocators.end())
			{
				newCopyStorageLocators.push_back(locator);
			}
			else
			{
				newCreatedStorageLocators.push_back(locator);
			}
		}

		//Add the entity to new storage
		newStorage->AddEntityToStorage(entity);
		// Copy all components to new storage.
		for (const auto& destination : newCopyStorageLocators)
		{
			auto origin = *std::ranges::find(copyLocators, destination.ComponentType, &ComponentStorageLocator::ComponentType);
			auto originStorage = oldStorage->GetComponentStoragePointer(origin.ComponentType);
			auto destinationStorage = newStorage->GetComponentStoragePointer(destination.ComponentType);
			if (!WorldComponentManager->CopyComponentToOtherStorage(originStorage, destinationStorage, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component copy to new storage failed!");
			};
			if (!WorldComponentManager->RemoveComponent(originStorage, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component removing from old storage failed!");
			};
		}
		// Delete all old components that were deleted
		for (const auto& type : deleteTypes)
		{
			auto originStorage = oldStorage->GetComponentStoragePointer(type);
			if (!WorldComponentManager->RemoveComponent(originStorage, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component removing from old storage failed!");
			};
		}
		// Delete all old components that were replaced instead of copied or added
		for (const auto& type : replacedTypes)
		{
			auto originStorage = oldStorage->GetComponentStoragePointer(type);
			if (!WorldComponentManager->RemoveComponent(originStorage, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component removing from old storage failed!");
			};
		}

		// Add the new components to their storages
		(AddComponentToNewStorage(newComponents, newCreatedStorageLocators), ...);
		// Delete entity from old storages.
		oldStorage->RemoveEntity(entity);

		auto it = EntityTypeMap.find(entity);
		it->second = newStorage->GetStorageEntityType();

		return true;
	}

	template<typename... DeleteComponentTypes, typename... NewComponentTypes>
	EntityType ModifyEntityComponents(const Entity entity, const EntityType entityType, NewComponentTypes... newComponents)
	{
		auto oldStorage = GetStorageByEntityType(entityType);
		auto entityLocator = oldStorage->GetEntityStorageLocator(entity);
		if (entityLocator.Entity == -1)
		{
			return false;
		}
		auto locatorSpan = oldStorage->GetComponentStorageLocators();
		std::vector<ComponentStorageLocator> componentStorageLocators;
		componentStorageLocators.reserve(locatorSpan.size());
		std::ranges::copy(locatorSpan, std::back_inserter(componentStorageLocators));

		std::vector<std::type_index> deleteTypes = GatherTypeIndexes<DeleteComponentTypes...>();
		std::vector<std::type_index> replacedTypes;
		std::vector<std::type_index> newTypes = GatherTypeIndexes<NewComponentTypes...>();
		std::vector<std::type_index> newStorageTypes = newTypes;

		// Remove deleted components from copy list.
		auto copyLocators = componentStorageLocators;
		for (auto type : deleteTypes)
		{
			auto it = std::ranges::find(copyLocators, type, &ComponentStorageLocator::ComponentType);
			copyLocators.erase(it);
		}
		// Gather component types that are replaced
		for (auto locator : copyLocators)
		{
			auto it = std::ranges::find(newTypes, locator.ComponentType);
			if (it != newTypes.end())
			{
				replacedTypes.push_back(*it);
			}
		}
		// Gather component types that are copied
		for (auto type : newTypes)
		{
			auto it = std::ranges::find(copyLocators, type, &ComponentStorageLocator::ComponentType);
			if (it != copyLocators.end())
			{
				copyLocators.erase(it);
			}
		}
		// Gather component types the new storage will have
		for (auto locator : copyLocators)
		{
			if (std::ranges::find(newStorageTypes, locator.ComponentType) == newStorageTypes.end())
			{
				newStorageTypes.push_back(locator.ComponentType);
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
				newStorage->AddComponentType(newlocator, WorldComponentManager->GetComponentStoragePointer(newlocator));
			}
			(CreateNewStorages<NewComponentTypes>(newCreatedStorageLocators, newTypes), ...);
			for (auto locator : newCreatedStorageLocators)
			{
				newStorage->AddComponentType(locator, WorldComponentManager->GetComponentStoragePointer(locator));
			}
		}
		else
		{
			auto newStorageLocators = newStorage->GetComponentStorageLocators();
			for (const auto& locator : newStorageLocators)
			{
				auto it = std::ranges::find(copyLocators, locator.ComponentType, &ComponentStorageLocator::ComponentType);
				if (it != copyLocators.end())
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
			auto originStorage = oldStorage->GetComponentStoragePointer(origin.ComponentType);
			auto destinationStorage = newStorage->GetComponentStoragePointer(destination.ComponentType);
			if (!WorldComponentManager->CopyComponentToOtherStorage(originStorage, destinationStorage, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component copy to new storage failed!");
			};
			if (!WorldComponentManager->RemoveComponent(originStorage, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component removing from old storage failed!");
			};
		}
		// Delete all old components that were deleted
		for (const auto& type : deleteTypes)
		{
			auto originStorage = oldStorage->GetComponentStoragePointer(type);
			if (!WorldComponentManager->RemoveComponent(originStorage, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component removing from old storage failed!");
			};
		}
		// Delete all old components that were modified instead of copied or added
		for(const auto& type : replacedTypes)
		{
			auto originStorage = oldStorage->GetComponentStoragePointer(type);
			if (!WorldComponentManager->RemoveComponent(originStorage, entityLocator.ComponentIndex))
			{
				fmt::println("ERROR: Component removing from old storage failed!");
			};
		}

		// Add the new components to their storages
		(AddComponentToNewStorage(newComponents, newCreatedStorageLocators), ...);

		// Delete entity from old storages.
		oldStorage->RemoveEntity(entity);

		auto it = EntityTypeMap.find(entity);
		it->second = newStorage->GetStorageEntityType();

		return true;
	}


protected:

	template<typename ComponentType>
	void AddComponentToNewStorage(const ComponentType component, const std::vector<ComponentStorageLocator>& locators)
	{
		std::type_index type = typeid(ComponentType);
		auto it = std::ranges::find(locators, type, &ComponentStorageLocator::ComponentType);
		WorldComponentManager->CreateNewComponentToStorage(*it, component);
	}


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
		auto locator = WorldComponentManager->CreateComponentStorage<ComponentType>();
		entityStoragePtr->AddComponentType(locator, WorldComponentManager->GetComponentStoragePointer(locator));
	}

	EntityTypeStorage* CreateEntityTypeStorage(const std::string& entityTypeName)
	{
		fmt::println("Creating new EntityType with name: {}", entityTypeName);

		EntityType newTypeID = EntityTypesCreated++;
		auto newStorage = std::make_unique<EntityTypeStorage>(newTypeID);
		EntityTypeNames.insert(std::pair(newTypeID, entityTypeName));
		const auto storageIt = EntityStorages.insert(std::pair(newTypeID, std::move(newStorage)));

		CurrentWorldCompositionNumber_ = WorldSharedBetweenManagers->SignalCompositionChange();
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

	std::unordered_map<Entity, EntityType> EntityTypeMap;
	std::map<EntityType, std::string> EntityTypeNames;
	std::map<EntityType, std::unique_ptr<EntityTypeStorage>> EntityStorages;

	SharedBetweenManagers* WorldSharedBetweenManagers;
	ComponentManager* WorldComponentManager;

	size_t EntitiesCreated = 0;
	size_t CurrentEntityCount = 0;
	size_t EntityTypesCreated = 0;

	// Used mainly to signal other managers to rebuild critical structures
	size_t CurrentWorldCompositionNumber_ = 0;

	std::mutex DestructiveEditMutex;
};
