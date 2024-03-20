module;
#include <fmt/core.h>
#include <array>

export module EncosyCore.System;

export import EncosyCore.SystemBase;
import EncosyCore.Entity;
import EncosyCore.ComponentManager;
import EncosyCore.EntityManager;
import EncosyCore.EntityTypeStorage;
import EncosyCore.ComponentTypeStorage;
import EncosyCore.ThreadedTaskRunner;

import <vector>;
import <span>;
import <map>;
import <string>;
import <concepts>;
import <unordered_set>;
import <thread>;
import <iterator>;
import <algorithm>;
import <functional>;
import <typeindex>;
import <typeinfo>;
import <mutex>;


export class System : public SystemBase
{
	friend class SystemManager;

public:
	System() {}
	~System() {}

protected:

	virtual void Init() = 0;
	virtual void PreUpdate(const double deltaTime) = 0;
	virtual void Update(const double deltaTime) = 0;
	virtual void PostUpdate(const double deltaTime) = 0;
	virtual void Destroy() = 0;


	virtual void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) = 0;

	template <typename ComponentType>
	void AddComponentQueryForReading(ReadOnlyComponentStorage<ComponentType>* storage)
	{
		AddReadOnlyComponentFetcher(&storage->Storage);
	}

	template <typename ComponentType>
	void AddComponentQueryForWriting(WriteReadComponentStorage<ComponentType>* storage)
	{
		AddWriteReadComponentFetcher(&storage->Storage);
	}

	template <typename ComponentType>
	void AddSystemDataForReading(ReadOnlySystemDataStorage<ComponentType>* storage)
	{
		AddReadOnlySystemDataFetcher(&storage->Storage);
	}

	template <typename ComponentType>
	void AddSystemDataForWriting(WriteReadSystemDataStorage<ComponentType>* storage)
	{
		AddWriteReadSystemDataFetcher(&storage->Storage);
	}

	template <typename ComponentType>
	void AddEntitiesForReading(EntityType typeID, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		AddReadOnlyAlwaysFetchEntityType(typeID, &storage->Storage);
	}

	template <typename ComponentType>
	void AddEntitiesForWriting(EntityType typeID, WriteReadAlwaysFetchedStorage<ComponentType>* storage)
	{
		AddWriteReadAlwaysFetchEntityType(typeID, &storage->Storage);
	}

	template <typename ComponentType>
	void AddComponentsForReading(ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		AddReadOnlyAlwaysFetchComponents(&storage->Storage);
	}

	template <typename ComponentType>
	void AddComponentsForWriting(WriteReadAlwaysFetchedStorage<ComponentType>* storage)
	{
		AddWriteReadAlwaysFetchComponents(&storage->Storage);
	}

	template <typename ComponentType>
	ComponentType GetCurrentEntityComponent(ReadOnlyComponentStorage<ComponentType>* storage)
	{
		return storage->Storage[RuntimeThreadInfo.outerIndex][RuntimeThreadInfo.innerIndexRead];
	}

	template <typename ComponentType>
	ComponentType& GetCurrentEntityComponent(WriteReadComponentStorage<ComponentType>* storage)
	{
		return storage->Storage[RuntimeThreadInfo.outerIndex][RuntimeThreadInfo.innerIndexWrite];
	}

	template <typename ComponentType>
	ComponentType& GetSystemData(WriteReadSystemDataStorage<ComponentType>* storage)
	{
		return storage->Storage[0];
	}

	template <typename ComponentType>
	ComponentType GetSystemData(ReadOnlySystemDataStorage<ComponentType>* storage)
	{
		return storage->Storage[0];
	}

	template <typename ComponentType>
	ComponentType GetEntityComponent(Entity entity, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		size_t index = WorldEntityManager->GetEntityComponentIndex(entity);
		return storage->Storage[index];
	}

	template <typename ComponentType>
	ComponentType GetEntityComponent(Entity entity, EntityType entityType, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		size_t index = WorldEntityManager->GetEntityComponentIndex(entity, entityType);
		return storage->Storage[index];
	}

	template <typename ComponentType>
	ComponentType& GetEntityComponent(Entity entity, WriteReadAlwaysFetchedStorage<ComponentType>* storage)
	{
		size_t index = WorldEntityManager->GetEntityComponentIndex(entity);
		return storage->Storage[index];
	}

	template <typename ComponentType>
	ComponentType& GetEntityComponent(Entity entity, EntityType entityType, WriteReadAlwaysFetchedStorage<ComponentType>* storage)
	{
		size_t index = WorldEntityManager->GetEntityComponentIndex(entity, entityType);
		return storage->Storage[index];
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromEntity(Entity entity)
	{
		return WorldEntityManager->GetReadOnlyComponentFromEntity<ComponentType>(entity);
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromEntity(Entity entity, EntityType entityType)
	{
		return WorldEntityManager->GetReadOnlyComponentFromEntity<ComponentType>(entity, entityType);
	}

	template<typename ComponentType>
	ComponentType* GetWriteReadComponentFromEntity(Entity entity)
	{
		return WorldEntityManager->GetWriteReadComponentFromEntity<ComponentType>(entity);
	}

	template<typename ComponentType>
	ComponentType* GetWriteReadComponentFromEntity(Entity entity, EntityType entityType)
	{
		return WorldEntityManager->GetWriteReadComponentFromEntity<ComponentType>(entity, entityType);
	}

	EntityOperationResult GetEntityTypeInfo(const std::string& entityTypeName)
	{
		return WorldEntityManager->GetEntityTypeInfo(entityTypeName);
	}

	template<typename ComponentType>
	bool HasComponentType(const Entity entity)
	{
		return WorldEntityManager->HasComponentType<ComponentType>(entity);
	}

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const EntityType entityType, const ComponentTypes... components)
	{
		return WorldEntityManager->CreateEntityWithData(entityType, components...);
	}

	bool DeleteEntity(const Entity entity) const
	{
		return WorldEntityManager->DeleteEntity(entity);
	}

	template<typename... DeleteComponentTypes, typename... NewComponentTypes>
	bool ModifyEntityComponents(const Entity entity, const EntityType entityType, NewComponentTypes... components)
	{
		EntityType newType = WorldEntityManager->ModifyEntityComponents<DeleteComponentTypes..., NewComponentTypes...>(entity, entityType, components...);
		return newType;
	}


	template<typename ComponentType>
	void CreateNewComponentToStorage(const ComponentStorageLocator locator, const ComponentType component)
	{
		WorldComponentManager->CreateNewComponentToStorage(locator, component);
	}

	void ResetComponentStorage(const ComponentStorageLocator locator) const
	{
		WorldComponentManager->ResetComponentStorage(locator);
	}


private:
	void SystemInit() override
	{
		
	}

	void SystemPreUpdate(const double deltaTime) override
	{
		CurrentDeltaTime = deltaTime;
		PreUpdate(CurrentDeltaTime);
	}

	void SystemUpdate() override
	{
		UpdateMatchingEntityTypes();
		FetchRequiredSpans();
		FetchedEntitiesInfo = WorldEntityManager->GetEntityFetchInfo(MatchingEntityTypes);

		Update(CurrentDeltaTime);	
	}

	void SystemPerEntityUpdate() override
	{
		for (size_t vectorIndex = 0; vectorIndex < FetchedEntitiesInfo.size(); vectorIndex++)
		{
			auto info = FetchedEntitiesInfo[vectorIndex];
			for (size_t spanIndex = 0; spanIndex < info.EntityLocators.size(); spanIndex++)
			{
				auto locator = info.EntityLocators[spanIndex];
				RuntimeThreadInfo.outerIndex = vectorIndex;
				RuntimeThreadInfo.innerIndexWrite = spanIndex;
				RuntimeThreadInfo.innerIndexRead = spanIndex;
				UpdatePerEntity(CurrentDeltaTime, locator.Entity, info.Type);
			}
		}
	}

	void SystemPostUpdate() override
	{
		PostUpdate(CurrentDeltaTime);
	}


private:
	SystemThreadInfo RuntimeThreadInfo;
};

export
template <typename T>
concept SystemClass = std::derived_from<T, System>;
