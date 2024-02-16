module;

export module ECS.System;

import ECS.ComponentManager;
import ECS.Entity;
import ECS.EntityManager;

import <string>;
import <concepts>;
import <vector>;
import <typeindex>;
import <typeinfo>;
import <map>;
import <span>;
import <unordered_set>;
import <functional>;


struct EntityFetchInfo
{
	EntitySpanFetchInfo fetchInfo;
	std::type_index componentType;
};

export typedef size_t SystemID;
export enum class SystemType { System = 0, RenderSystem, PhysicsSystem };

export class System
{
	friend class SystemManager;

public:
	System() {}
	~System() {}

	SystemType GetType() const { return Type; }
	int GetID() const { return ID; }
	bool GetInitialized() const { return bInitialized; }
	bool GetEnabled() const { return bEnabled; }

protected:

	virtual void Init() = 0;
	virtual void PreUpdate(float deltaTime) = 0;
	virtual void Update(float deltaTime) = 0;
	//virtual void UpdatePerEntityType(float deltaTime, EntityTypeID typeID, size_t entityCount, size_t storageIndex) = 0;
	virtual void UpdatePerEntity(float deltaTime, Entity entity, size_t vectorIndex, size_t spanIndex) = 0;
	virtual void PostUpdate(float deltaTime) = 0;
	virtual void Destroy() = 0;

	virtual void SystemUpdate(float deltaTime)
	{
		UpdateMatchingEntityTypes();
		FetchRequiredSpans();
		FetchedEntitiesInfo = WorldEntityManager->GetEntityFetchInfo(MatchingEntityTypes);
		Update(deltaTime);
		for (size_t vectorIndex = 0; vectorIndex < FetchedEntitiesInfo.size(); vectorIndex++)
		{
			auto info = FetchedEntitiesInfo[vectorIndex];
			//UpdatePerEntityType(deltaTime, info.TypeID, info.EntityCount, vectorIndex);
			for (size_t spanIndex = 0; spanIndex < info.EntityLocators.size(); spanIndex++)
			{
				auto locator = info.EntityLocators[spanIndex];
				UpdatePerEntity(deltaTime, { locator.ID, info.TypeID }, vectorIndex, spanIndex);
			}
		}
	}

	template <typename... ComponentTypes>
	void AddWantedComponentDataForReading(std::vector<std::span<ComponentTypes  const>>* ... storages)
	{
		(AddReadOnlyComponentFetcher(storages), ...);
	}

	template <typename ComponentType>
	void AddReadOnlyComponentFetcher(std::vector<std::span<ComponentType const>>* storage)
	{
		auto& id = typeid(ComponentType);
		WantedReadOnlyComponentTypes.insert(id);
		
		std::function<void()>  fetcher = [=]() {
			ReadOnlyComponentDataFetcher<ComponentType>(storage);
			};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void ReadOnlyComponentDataFetcher(std::vector<std::span<ComponentType const>>* storage)
	{
		std::vector<std::span<ComponentType const>> newStorage;
		for (auto entityTypeId: MatchingEntityTypes)
		{
			std::span<ComponentType const> span = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(entityTypeId);
			newStorage.push_back(span);
		}
		*storage = newStorage;
	}

	template <typename... ComponentTypes>
	void AddWantedComponentDataForWriting(std::vector<std::span<ComponentTypes>>* ... storages)
	{
		(AddWriteReadComponentFetcher(storages), ...);
	}

	template <typename ComponentType>
	void AddWriteReadComponentFetcher(std::vector<std::span<ComponentType>>* storage)
	{
		auto& id = typeid(ComponentType);
		WantedWriteReadComponentTypes.insert(id);

		std::function<void()>  fetcher = [=]() {
			WriteReadComponentDataFetcher<ComponentType>(storage);
		};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void WriteReadComponentDataFetcher(std::vector<std::span<ComponentType>>* storage)
	{
		std::vector<std::span<ComponentType>> newStorage;
		for (auto entityTypeId: MatchingEntityTypes)
		{
			std::span<ComponentType> span = WorldEntityManager->GetEntityWriteReadComponentSpan<ComponentType>(entityTypeId);
			newStorage.push_back(span);
		}
		*storage = newStorage;
	}


	template <typename ComponentType>
	void AddSystemDataForReading(ComponentType* storage)
	{
		auto& id = typeid(ComponentType);
		ReadOnlySystemData.insert(id);
		
		std::function<void()>  fetcher = [=]() {
			ReadOnlySystemDataFetcher<ComponentType>(storage);
			};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void ReadOnlySystemDataFetcher(ComponentType* storage)
	{
		*storage = WorldComponentManager->GetReadOnlySystemData<ComponentType>();
	}

	template <typename ComponentType>
	void AddSystemDataForWriting(ComponentType* &storage)
	{
		auto& id = typeid(ComponentType);
		WriteReadSystemData.insert(id);

		std::function<void()>  fetcher = [&]() {
			WriteReadSystemDataFetcher<ComponentType>(storage);
			};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void WriteReadSystemDataFetcher(ComponentType* &storage)
	{
		storage = WorldComponentManager->GetWriteReadSystemData<ComponentType>();
	}

	template <typename ComponentType>
	void AddForbiddenComponentType()
	{
		auto& id = typeid(ComponentType);
		ForbiddenComponentTypes.insert(id);
	}

	template <typename... ComponentTypes>
	void AddAlwaysFetchedEntitiesForReading(EntityTypeID typeID, std::span<ComponentTypes  const>* ... storages)
	{
		(AddReadOnlyAlwaysFetch(typeID, storages), ...);
	}

	template <typename ComponentType>
	void AddReadOnlyAlwaysFetch(EntityTypeID typeID, std::span<ComponentType const>* storage)
	{
		auto& id = typeid(ComponentType);
		EntitySpanFetchInfo FetchedEntityInfo = WorldEntityManager->GetEntityFetchInfo(typeID);
		EntityFetchInfo info = { .fetchInfo = FetchedEntityInfo , .componentType = id };
		AlwaysFetchedEntityComponents.push_back(info);
		AlwaysFetchedReadOnlyComponentTypes.insert(id);

		std::function<void()>  fetcher = [=]() {
			ReadOnlyAlwaysFetchers<ComponentType>(typeID, storage);
			};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void ReadOnlyAlwaysFetchers(EntityTypeID typeID, std::span<ComponentType const>* span)
	{
		std::span<ComponentType const> fetchedSpan = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(typeID);
		*span = fetchedSpan;
	}

	template <typename... ComponentTypes>
	void AddAlwaysFetchedEntitiesForWriting(EntityTypeID typeID, std::span<ComponentTypes>* ... storages)
	{
		(AddWriteReadAlwaysFetch(typeID, storages), ...);
	}

	template <typename ComponentType>
	void AddWriteReadAlwaysFetch(EntityTypeID typeID, std::span<ComponentType>* storage)
	{
		auto& id = typeid(ComponentType);
		EntitySpanFetchInfo FetchedEntityInfo = WorldEntityManager->GetEntityFetchInfo(typeID);
		EntityFetchInfo info = { .fetchInfo = FetchedEntityInfo , .componentType = id };
		AlwaysFetchedEntityComponents.push_back(info);
		AlwaysFetchedWriteReadComponentTypes.insert(id);

		std::function<void()>  fetcher = [=]() {
			WriteReadAlwaysFetchers<ComponentType>(typeID, storage);
			};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void WriteReadAlwaysFetchers(EntityTypeID typeID, std::span<ComponentType>* span)
	{
		std::span<ComponentType> fetchedSpan = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(typeID);
		*span = fetchedSpan;
	}


	void FetchRequiredSpans()
	{
		for (auto& fetcher : FetchFunctions)
		{
			std::invoke(fetcher);
		}
	}

	void UpdateMatchingEntityTypes()
	{
		MatchingEntityTypes = WorldEntityManager->GetEntityTypesWithComponentConditions(WantedReadOnlyComponentTypes, WantedWriteReadComponentTypes, ForbiddenComponentTypes);
	}

	std::vector<EntitySpanFetchInfo> FetchedEntitiesInfo;

	void SetEnabled(bool enabled) { bEnabled = enabled; }
	void SetInitialized(bool initialized) { bInitialized = initialized; }

	SystemType Type = SystemType::System;
	int SystemQueueIndex = 1000;

	EntityManager* WorldEntityManager;
	ComponentManager* WorldComponentManager;

	std::vector<EntityTypeID> MatchingEntityTypes;

private:
	SystemID ID = -1;
	bool bInitialized = false;
	bool bEnabled = false;
	bool AllowThreading;
	

	std::vector<EntityFetchInfo> AlwaysFetchedEntityComponents;

	std::unordered_set<std::type_index> ReadOnlySystemData;
	std::unordered_set<std::type_index> WriteReadSystemData;
	std::unordered_set<std::type_index> WantedReadOnlyComponentTypes;
	std::unordered_set<std::type_index> WantedWriteReadComponentTypes;
	std::unordered_set<std::type_index> AlwaysFetchedReadOnlyComponentTypes;
	std::unordered_set<std::type_index> AlwaysFetchedWriteReadComponentTypes;
	
	std::unordered_set<std::type_index> ForbiddenComponentTypes;


	std::vector<std::function<void()>> FetchFunctions;
};

export
template <typename T>
concept SystemClass = std::is_base_of<System, T>::value;


export class ThreadedSystem : public System
{
	friend class SystemManager;

public:
	ThreadedSystem() {}
	~ThreadedSystem() {}


	virtual void Init() = 0;
	virtual void PreUpdate(float deltaTime) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void PostUpdate(float deltaTime) = 0;
	virtual void Destroy() = 0;

	virtual void SystemUpdate(float deltaTime) override
	{
		System::SystemUpdate(deltaTime);
		//UpdateMatchingEntityTypes();
		//FetchRequiredSpans();
		//FetchedEntitiesInfo = WorldEntityManager->GetEntityFetchInfo(MatchingEntityTypes);
		//Update(deltaTime);
		//for (size_t i = 0; i < FetchedEntitiesInfo.size(); i++)
		//{
		//	auto info = FetchedEntitiesInfo[i];
		//	//UpdatePerEntityType(deltaTime, info.TypeID, info.EntityCount, i);
		//}
	}

	virtual void ThreadTask()
	{

	}

	template <typename ComponentType>
	std::vector<ComponentType> CopyFromSpan(std::span<ComponentType> span)
	{
		std::vector<ComponentType> copyVector;
		copyVector.reserve(span.size());
		copyVector.assign(span.begin(), span.end());
		return copyVector;
	}

	template <typename ComponentType>
	void PushResults(std::vector<ComponentType>& componentVector)
	{

	}

private:
	int MinEntitiesPerThread = 8;

};

export
template <typename T>
concept ThreadedSystemClass = std::is_base_of<ThreadedSystem, T>::value;