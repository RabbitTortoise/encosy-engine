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
import <concepts>;


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
	bool ReplaceEntityComponentData(const Entity entity, const EntityType entityType, ComponentType&&... components)
	{
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR (EntityManager.ReplaceEntityComponentData): Could not find storage for entityType: {}", entityType);
			return false;
		}
		auto entityComponentIndex = storageIt->second->GetEntityComponentIndex(entity);
		(ReplaceEntityComponent(storageIt->second.get(), entityComponentIndex, std::forward<decltype(std::move(components))>(std::move(components))), ...);
		return true;
	}	

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const std::string entityTypeName, ComponentTypes&&... components)
	{
		return CreateEntityWithData(GetEntityTypeInfo(entityTypeName).Type, std::forward<decltype(std::move(components))>(std::move(components))...);
	}

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(ComponentTypes&&... components)
	{
		auto componentTypeIndexes = GatherTypeIndexes<std::remove_reference_t<decltype(components)>...>();
		EntityType entityType = FindEntityTypeWithComponents(componentTypeIndexes);

		return CreateEntityWithData(entityType, std::forward<decltype(std::move(components))>(std::move(components))...);
	}

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const EntityType entityType, ComponentTypes&&... components)
	{
		EntityOperationResult entityOperationResult = GetEntityTypeInfo(entityType);
		if (entityOperationResult.OperationResult == EntityOperationResult::Result::NotFound)
		{
			const std::string name = std::format("EntityType{}", EntityTypesCreated);
			entityOperationResult = CreateEntityType<std::remove_reference_t<ComponentTypes>...>(name);
		}
		auto storage = GetStorageByEntityType(entityOperationResult.Type);

		const Entity createdEntity = (EntitiesCreatedAtomic.exchange(EntitiesCreatedAtomic + 1));
		++CurrentEntityCountAtomic;

		storage->AddEntityToStorage(createdEntity);
		(CreateEntityComponent(storage, std::forward<decltype(std::move(components))>(std::move(components))), ...);
		return createdEntity;
	}

	template<typename...ComponentTypes>
	Entity CreateEntityWithDataThreaded(const EntityType entityType, ComponentTypes&&...components)
	{
		std::scoped_lock storageLock(*WorldSharedBetweenManagers->GetEntityStorageMutex(entityType));

		auto storage = GetStorageByEntityType(entityType);

		const Entity createdEntity = (EntitiesCreatedAtomic.exchange(EntitiesCreatedAtomic+1));
		++CurrentEntityCountAtomic;

		storage->AddEntityToStorage(createdEntity);
		(CreateEntityComponent(storage, std::forward<decltype(std::move(components))>(std::move(components))), ...);

		return createdEntity;
	}

	template<typename...ComponentTypes>
	EntityOperationResult CreateEntityType(const std::string& entityTypeName)
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
	void GetEntityTypesWithComponentConditions(
		std::vector<EntityType>& out,
		const std::unordered_set<std::type_index>& fetchedReadOnlyTypes,
		const std::unordered_set<std::type_index>& fetchedWriteReadTypes,
		const std::unordered_set<std::type_index>& requiredTypes,
		const std::unordered_set<std::type_index>& forbiddenTypes) const
	{

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

			bool requiredFound = true;
			for (auto const& type : requiredTypes)
			{
				requiredFound = (std::ranges::find(componentTypes, type) != componentTypes.end());
				if (!requiredFound) { requiredFound = false; break; }
			}
			if (!requiredFound) { continue; }

			bool wantedFound = true;
			for (auto const& type : fetchedReadOnlyTypes)
			{
				wantedFound = (std::ranges::find(componentTypes, type) != componentTypes.end());
				if (!wantedFound) { wantedFound = false; break; }
			}
			if (!wantedFound) { continue; }

			wantedFound = true;
			for (auto const& type : fetchedWriteReadTypes)
			{
				wantedFound = (std::ranges::find(componentTypes, type) != componentTypes.end());
				if (!wantedFound) { wantedFound = false; break; }
			}
			if (!wantedFound) { continue; }

			out.emplace_back(pair.first);
		}
	}

	std::vector<EntityType> GetEntityTypesWithComponent(const std::type_index componentType) const
	{
		std::vector<EntityType> entityTypes;

		for (auto const& pair : EntityStorages)
		{
			auto componentTypes = pair.second->GetEntityComponentTypes();
			if(std::ranges::find(componentTypes, componentType) != componentTypes.end())
			{
				entityTypes.emplace_back(pair.first);
			}
		}
		return entityTypes;
	}

	EntitySpanFetchInfo GetEntityFetchInfo(const EntityType entityType) const
	{
		const auto storage = GetStorageByEntityType(entityType);
		EntitySpanFetchInfo info = { .Type = entityType, .TypeName = EntityTypeNames.find(entityType)->second, .EntityCount = storage->GetEntityCount(), .EntityLocators = storage->GetEntityStorageLocators()};
		return info;
	}

	std::vector<EntitySpanFetchInfo> GetEntityFetchInfo(const std::vector<EntityType>& entityTypes) const
	{
		std::vector<EntitySpanFetchInfo> info;
		info.reserve(entityTypes.size());

		for (auto entityType : entityTypes)
		{
			const auto storage = GetStorageByEntityType(entityType);
			EntitySpanFetchInfo i = { .Type = entityType, .TypeName = EntityTypeNames.find(entityType)->second, .EntityCount = storage->GetEntityCount(), .EntityLocators = storage->GetEntityStorageLocators() };
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
		auto const& storage = GetStorageByEntityType(entityType);
		return storage->GetEntityFromComponentIndex(componentIndex);
	}

	size_t GetCurrentEntityCount() const
	{
		return CurrentEntityCountAtomic.load();
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromEntity(Entity entity, EntityType entityType)
	{
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Can't find type in EntityTypeMap: {}", entity, entityType);
			return {};
		}
		const auto storage = storageIt->second.get();
		auto componentStorage = storage->GetComponentStoragePointer<ComponentType>();
		std::scoped_lock storageLock(*WorldSharedBetweenManagers->GetEntityStorageMutex(entityType));
		const EntityComponentIndex index = storage->GetEntityComponentIndex(entity);
		return WorldComponentManager->GetReadOnlyComponentFromStorage<ComponentType>(componentStorage, index);
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromEntityThreaded(Entity entity)
	{
		const EntityType entityType = GetEntityTypeThreaded(entity);
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Can't find type in EntityTypeMap: {}", entity, entityType);
			return {};
		}
		const auto storage = storageIt->second.get();
		auto componentStorage = storage->GetComponentStoragePointer<ComponentType>();
		std::scoped_lock storageLock(*WorldSharedBetweenManagers->GetEntityStorageMutex(entityType));
		const EntityComponentIndex index = storage->GetEntityComponentIndex(entity);
		return WorldComponentManager->GetReadOnlyComponentFromStorageThreaded<ComponentType>(componentStorage, index);
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromEntityThreaded(Entity entity, const EntityType entityType)
	{
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Can't find type in EntityTypeMap: {}", entity, entityType);
			return {};
		}
		const auto storage = storageIt->second.get();
		auto componentStorage = storage->GetComponentStoragePointer<ComponentType>();
		const EntityComponentIndex index = storage->GetEntityComponentIndex(entity);
		return WorldComponentManager->GetReadOnlyComponentFromStorageThreaded<ComponentType>(componentStorage, index);
	}

	template<typename ComponentType>
	ComponentType* GetWriteReadComponentFromEntity(Entity entity, EntityType entityType)
	{
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
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

	EntityComponentIndex GetEntityComponentIndex(Entity entity)
	{
		const auto storage = GetStorageByEntity(entity);
		return storage->GetEntityComponentIndex(entity);
	};

	EntityComponentIndex GetEntityComponentIndex(Entity entity, EntityType entityType)
	{
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: No storage found for given EntityType", entity, entityType);
			return {};
		}
		const auto storage = storageIt->second.get();
		return storage->GetEntityComponentIndex(entity);
	};


	bool DeleteEntityThreaded(const Entity entity, EntityType entityType)
	{
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: No storage found for given EntityType", entity, entityType);
			return false;
		}
		const auto storage = storageIt->second.get();

		std::scoped_lock storageLock(*WorldSharedBetweenManagers->GetEntityStorageMutex(entityType));

		const auto entityComponentIndex = storage->GetEntityComponentIndex(entity);
		const auto componentStorageLocators = storage->GetComponentStorageLocators();
		if (storage->RemoveEntity(entity))
		{
			for (const auto& locator : componentStorageLocators)
			{
				auto componentStorage = storage->GetComponentStoragePointer(locator.ComponentType);
				WorldComponentManager->RemoveComponent(componentStorage, entityComponentIndex);
			}

			--CurrentEntityCountAtomic;
			return true;
		}
		return false;

	}

	bool DeleteEntity(const Entity entity)
	{
		return DeleteEntity(entity, GetEntityType(entity));
	}

	bool DeleteEntity(const Entity entity, const EntityType entityType)
	{
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: No storage found for given EntityType", entity, entityType);
			return false;
		}
		const auto storage = storageIt->second.get();
		const auto entityComponentIndex = storage->GetEntityComponentIndex(entity);
		const auto componentStorageLocators = storage->GetComponentStorageLocators();
		if (storage->RemoveEntity(entity))
		{
			for (const auto& locator : componentStorageLocators)
			{
				auto componentStorage = storage->GetComponentStoragePointer(locator.ComponentType);
				WorldComponentManager->RemoveComponent(componentStorage, entityComponentIndex);
			}
			--CurrentEntityCountAtomic;
			return true;
		}
		return false;
	}

	template<typename ComponentType>
	bool HasComponentType(const Entity entity) const
	{
		return HasComponentType<ComponentType>(entity, GetEntityType(entity));
	}

	template<typename ComponentType>
	bool HasComponentType(const Entity entity, const EntityType entityType) const
	{
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: No storage found for given EntityType", entity, entityType);
			return false;
		}
		const auto storage = storageIt->second.get();
		return storage->HasComponentType<ComponentType>();
	}

	template<typename ComponentType>
	bool HasComponentTypeThreaded(const Entity entity)
	{
		const auto entityType = GetEntityTypeThreaded(entity);
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: No storage found for given EntityType", entity, entityType);
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
	bool ModifyEntityComponentsThreaded(const Entity entity, const EntityType entityType, const EntityType newEntityType, NewComponentTypes&&... newComponents)
	{
		std::scoped_lock lock(*WorldSharedBetweenManagers->GetEntityStorageMutex(entityType), *WorldSharedBetweenManagers->GetEntityStorageMutex(newEntityType));

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
		for (const auto& type : deleteTypes)
		{
			auto it = std::ranges::find(copyLocators, type, &ComponentStorageLocator::ComponentType);
			copyLocators.erase(it);
		}
		// Gather component types that are replaced
		for (const auto& locator : copyLocators)
		{
			auto it = std::ranges::find(newTypes, locator.ComponentType);
			if (it != newTypes.end())
			{
				replacedTypes.emplace_back(*it);
			}
		}
		// Gather component types that are copied
		for (const auto& type : newTypes)
		{
			auto it = std::ranges::find(copyLocators, type, &ComponentStorageLocator::ComponentType);
			if (it != copyLocators.end())
			{
				copyLocators.erase(it);
			}
		}
		// Gather component types the new storage will have
		for (const auto& locator : copyLocators)
		{
			if (std::ranges::find(newStorageTypes, locator.ComponentType) == newStorageTypes.end())
			{
				newStorageTypes.emplace_back(locator.ComponentType);
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
		auto foundNewEntityType = newStorage->GetStorageEntityType();
		if(foundNewEntityType != newEntityType)
		{
			fmt::println("ERROR: ModifyEntityComponentsThreaded(): The requested modifications did not transform the entity to correct type! New {}, expected {}", foundNewEntityType, newEntityType);
			return false;
		}

		auto newStorageLocators = newStorage->GetComponentStorageLocators();
		for (const auto& locator : newStorageLocators)
		{
			if (std::ranges::find(copyLocators, locator.ComponentType, &ComponentStorageLocator::ComponentType) != copyLocators.end())
			{
				newCopyStorageLocators.emplace_back(locator);
			}
			else
			{
				newCreatedStorageLocators.emplace_back(locator);
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
			if (!WorldComponentManager->MoveComponentToOtherStorage(originStorage, destinationStorage, entityLocator.ComponentIndex))
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
		(AddComponentToNewStorage(std::forward<decltype(std::move(newComponents))>(std::move(newComponents)), newCreatedStorageLocators), ...);
		// Delete entity from old storages.
		oldStorage->RemoveEntity(entity);

		return true;
	}

	template<typename... DeleteComponentTypes, typename... NewComponentTypes>
	bool ModifyEntityComponents(const Entity entity, const EntityType entityType, NewComponentTypes&&... newComponents)
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
		for (const auto& type : deleteTypes)
		{
			auto it = std::ranges::find(copyLocators, type, &ComponentStorageLocator::ComponentType);
			copyLocators.erase(it);
		}
		// Gather component types that are replaced
		for (const auto& locator : copyLocators)
		{
			auto it = std::ranges::find(newTypes, locator.ComponentType);
			if (it != newTypes.end())
			{
				replacedTypes.emplace_back(*it);
			}
		}
		// Gather component types that are copied
		for (const auto& type : newTypes)
		{
			auto it = std::ranges::find(copyLocators, type, &ComponentStorageLocator::ComponentType);
			if (it != copyLocators.end())
			{
				copyLocators.erase(it);
			}
		}
		// Gather component types the new storage will have
		for (const auto& locator : copyLocators)
		{
			if (std::ranges::find(newStorageTypes, locator.ComponentType) == newStorageTypes.end())
			{
				newStorageTypes.emplace_back(locator.ComponentType);
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
				newCopyStorageLocators.emplace_back(newlocator);
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
					newCopyStorageLocators.emplace_back(locator);
				}
				else
				{
					newCreatedStorageLocators.emplace_back(locator);
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
			if (!WorldComponentManager->MoveComponentToOtherStorage(originStorage, destinationStorage, entityLocator.ComponentIndex))
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
		(AddComponentToNewStorage(std::forward<decltype(std::move(newComponents))>(std::move(newComponents)), newCreatedStorageLocators), ...);

		// Delete entity from old storages.
		oldStorage->RemoveEntity(entity);

		return true;
	}


protected:

	template<typename ComponentType>
	void AddComponentToNewStorage(ComponentType&& component, const std::vector<ComponentStorageLocator>& locators)
	{
		std::type_index type = typeid(ComponentType);
		auto it = std::ranges::find(locators, type, &ComponentStorageLocator::ComponentType);
		WorldComponentManager->CreateNewComponentToStorage(*it, std::forward<decltype(std::move(component))>(std::move(component)));
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
		EntityType entityType = GetEntityType(entity);
		if(entityType == -1)
		{
			fmt::println("ERROR: Entity with ID {} was not found", entity);
			return {};
		}
		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Entity {} has invalid type in EntityTypeMap: {}", entity, entityType);
			return {};
		}
		return storageIt->second.get();
	}

	EntityTypeStorage* GetStorageByEntityType(const EntityType entityType) const
	{

		auto const storageIt = std::ranges::find(EntityStorages, entityType, &std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>::first);
		if (storageIt == EntityStorages.end())
		{
			fmt::println("ERROR: Type not found from EntityTypeMap: {}", entityType);
			return {};
		}
		return storageIt->second.get();
	}

	template<typename ComponentType>
	void CreateEntityComponent(EntityTypeStorage* storage, ComponentType&& component)
	{
		auto componentStoragePointer = storage->GetComponentStoragePointer<ComponentType>();
		WorldComponentManager->CreateNewComponentToStorage(componentStoragePointer, std::forward<decltype(std::move(component))>(std::move(component)));
	}

	template<typename ComponentType>
	void ReplaceEntityComponent(EntityTypeStorage* storage, const EntityComponentIndex entityComponentIndex, ComponentType&& component)
	{
		auto componentStoragePointer = storage->GetComponentStoragePointer<ComponentType>();
		WorldComponentManager->ReplaceComponentData(componentStoragePointer, entityComponentIndex, std::forward<decltype(std::move(component))>(std::move(component)));
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
		EntityStorages.emplace_back(std::pair(newTypeID, std::move(newStorage)));
		WorldSharedBetweenManagers->AddEntityStorageMutex(newTypeID);

		CurrentWorldCompositionNumber_ = WorldSharedBetweenManagers->SignalCompositionChange();
		return EntityStorages.back().second.get();
	}

	EntityType GetEntityType(const Entity entity) const
	{
		EntityType entityType = -1;
		for (const auto& entityStorage : EntityStorages)
		{
			if (entityStorage.second.get()->ContainsEntity(entity))
			{
				entityType = entityStorage.second->GetStorageEntityType();
				break;
			}
		}
		return entityType;
	}

	EntityType GetEntityTypeThreaded(const Entity entity) const
	{
		EntityType entityType = -1;
		for (const auto& entityStorage : EntityStorages)
		{
			entityType = entityStorage.second->GetStorageEntityType();
			std::scoped_lock lock(*WorldSharedBetweenManagers->GetEntityStorageMutex(entityType));
			if (entityStorage.second.get()->ContainsEntity(entity))
			{
				break;
			}
		}
		return entityType;
	}

	template<typename ComponentType>
	static void AddTypeIndexToVector(std::vector<std::type_index>& indexVector)
	{
		indexVector.emplace_back(typeid(ComponentType));
	}

	template<typename...ComponentTypes>
	static std::vector<std::type_index> GatherTypeIndexes()
	{
		std::vector<std::type_index> componentTypeIndexes;
		(AddTypeIndexToVector<ComponentTypes>(componentTypeIndexes), ...);
		std::ranges::sort(componentTypeIndexes, {});
		return componentTypeIndexes;
	}

	EntityType FindEntityTypeWithComponents(const std::vector<std::type_index>& indexVector)
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

	std::map<EntityType, std::string> EntityTypeNames;
	std::vector<std::pair<EntityType, std::unique_ptr<EntityTypeStorage>>> EntityStorages;
	SharedBetweenManagers* WorldSharedBetweenManagers;
	ComponentManager* WorldComponentManager;

	std::mutex ThreadProtectionMutex;
	std::atomic<size_t> EntitiesCreatedAtomic = 0;
	std::atomic<size_t> CurrentEntityCountAtomic = 0;
	size_t EntityTypesCreated = 0;

	// Used mainly to signal other managers to rebuild critical structures
	size_t CurrentWorldCompositionNumber_ = 0;
};
