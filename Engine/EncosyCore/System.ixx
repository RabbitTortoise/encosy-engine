module;
#include <fmt/core.h>
#include <array>

export module ECS.System;

import ECS.ComponentManager;
import ECS.Entity;
import ECS.EntityManager;
import EncosyCore.ThreadedTaskRunner;

import <string>;
import <concepts>;
import <vector>;
import <typeindex>;
import <typeinfo>;
import <map>;
import <span>;
import <unordered_set>;
import <functional>;
import <algorithm>;




export typedef size_t SystemID;
export enum class SystemType { System = 0, RenderSystem, PhysicsSystem, ThreadedSystem };

export
template <typename ComponentType>
struct ReadOnlyComponentStorage
{
	std::vector<std::span<ComponentType const>> Storage;
};
export
template <typename ComponentType>
struct WriteReadComponentStorage
{
	std::vector<std::span<ComponentType>> Storage;
};
export
template <typename ComponentType>
struct ReadOnlySystemDataStorage
{
	std::span<ComponentType const> Storage;
};
export
template <typename ComponentType>
struct WriteReadSystemDataStorage
{
	std::span<ComponentType> Storage;
};
export
template <typename ComponentType>
struct ReadOnlyAlwaysFetchedStorage
{
	std::span<ComponentType const> Storage;
};
export
template <typename ComponentType>
struct WriteReadAlwaysFetchedStorage
{
	std::span<ComponentType> Storage;
};


export
template <typename ComponentType>
struct ThreadComponentStorage
{
	std::vector<std::vector<std::vector<ComponentType>>> Storage;
};
struct EntityFetchInfo
{
	EntitySpanFetchInfo fetchInfo;
	std::type_index componentType;
};

export struct ThreadInfo
{
	size_t outerIndex = 0;
	size_t innerIndexRead = 0;
	size_t innerIndexWrite = 0;
	std::array<size_t, 5> padding = {};
};

export class SystemBase
{
	friend class SystemManager;
	friend class System;
	friend class ThreadedSystem;

public:

	SystemBase() {}
	~SystemBase() {}

	SystemType GetType() const { return Type; }
	int GetID() const { return ID; }
	bool GetInitialized() const { return bInitialized; }
	bool GetEnabled() const { return bEnabled; }

protected:

	virtual void Init() = 0;
	virtual void PreUpdate(float deltaTime) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void PostUpdate(float deltaTime) = 0;
	virtual void Destroy() = 0;

	template <typename ComponentType>
	void AddForbiddenComponentType()
	{
		auto& id = typeid(ComponentType);
		ForbiddenComponentTypes.insert(id);
	}

private:
	virtual void SystemUpdate(float deltaTime) = 0;
	void SetInitialized(bool initialized) { bInitialized = initialized; }
	void SetEnabled(bool enabled) { bEnabled = enabled; }

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
		for (auto entityType : MatchingEntityTypes)
		{
			std::span<ComponentType> span = WorldEntityManager->GetEntityWriteReadComponentSpan<ComponentType>(entityType);
			newStorage.push_back(span);
		}
		if (MatchingEntityTypes.size() == 0)
		{
			WorldComponentManager->RecordWriteReadComponentAccess<ComponentType>();
		}
		*storage = newStorage;
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
		for (auto entityType : MatchingEntityTypes)
		{
			std::span<ComponentType const> span = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(entityType);
			newStorage.push_back(span);
		}
		if (MatchingEntityTypes.size() == 0)
		{
			WorldComponentManager->RecordReadOnlyComponentAccess<ComponentType>();
		}
		*storage = newStorage;
	}

	template <typename ComponentType>
	void AddWriteReadAlwaysFetch(EntityType typeID, std::span<ComponentType>* storage)
	{
		auto& id = typeid(ComponentType);
		EntitySpanFetchInfo FetchedEntityInfo = WorldEntityManager->GetEntityFetchInfo(typeID);
		EntityFetchInfo info = { .fetchInfo = FetchedEntityInfo , .componentType = id };
		AlwaysFetchedEntityComponents.push_back(info);
		AlwaysFetchedWriteReadComponentTypes.insert(id);

		std::function<void()>  fetcher = [=]() {
			WriteReadAlwaysFetcher<ComponentType>(typeID, storage);
			};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void WriteReadAlwaysFetcher(EntityType typeID, std::span<ComponentType>* span)
	{
		std::span<ComponentType> fetchedSpan = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(typeID);
		*span = fetchedSpan;
	}


	template <typename ComponentType>
	void AddReadOnlyAlwaysFetch(EntityType typeID, std::span<ComponentType const>* storage)
	{
		auto& id = typeid(ComponentType);
		EntitySpanFetchInfo FetchedEntityInfo = WorldEntityManager->GetEntityFetchInfo(typeID);
		EntityFetchInfo info = { .fetchInfo = FetchedEntityInfo , .componentType = id };
		AlwaysFetchedEntityComponents.push_back(info);
		AlwaysFetchedReadOnlyComponentTypes.insert(id);

		std::function<void()>  fetcher = [=]() {
			ReadOnlyAlwaysFetcher<ComponentType>(typeID, storage);
			};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void ReadOnlyAlwaysFetcher(EntityType typeID, std::span<ComponentType const>* span)
	{
		std::span<ComponentType const> fetchedSpan = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(typeID);
		*span = fetchedSpan;
	}

	template <typename ComponentType>
	void AddReadOnlySystemDataFetcher(std::span<ComponentType const>* span)
	{
		auto& id = typeid(ComponentType);
		ReadOnlySystemData.insert(id);

		std::function<void()>  fetcher = [=]() {
			ReadOnlySystemDataFetcher<ComponentType>(span);
			};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void AddWriteReadSystemDataFetcher(std::span<ComponentType>* span)
	{
		auto& id = typeid(ComponentType);
		WriteReadSystemData.insert(id);

		std::function<void()>  fetcher = [=]() {
			WriteReadSystemDataFetcher<ComponentType>(span);
			};

		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void WriteReadSystemDataFetcher(std::span<ComponentType>* span)
	{
		*span = WorldComponentManager->GetWriteReadSystemData<ComponentType>();
	}

	template <typename ComponentType>
	void ReadOnlySystemDataFetcher(std::span<ComponentType const>* span)
	{
		*span = WorldComponentManager->GetReadOnlySystemData<ComponentType>();
	}

// Variables:
protected:

	SystemType Type = SystemType::System;
	int SystemQueueIndex = 1000;

	EntityManager* WorldEntityManager;
	ComponentManager* WorldComponentManager;


	std::vector<EntitySpanFetchInfo> FetchedEntitiesInfo;
	std::vector<EntityType> MatchingEntityTypes;

private:
	std::vector<EntityFetchInfo> AlwaysFetchedEntityComponents;
	std::unordered_set<std::type_index> ReadOnlySystemData;
	std::unordered_set<std::type_index> WriteReadSystemData;
	std::unordered_set<std::type_index> WantedReadOnlyComponentTypes;
	std::unordered_set<std::type_index> WantedWriteReadComponentTypes;
	std::unordered_set<std::type_index> AlwaysFetchedReadOnlyComponentTypes;
	std::unordered_set<std::type_index> AlwaysFetchedWriteReadComponentTypes;
	std::unordered_set<std::type_index> ForbiddenComponentTypes;
	std::vector<std::function<void()>> FetchFunctions;

	SystemID ID = -1;
	bool bInitialized = false;
	bool bEnabled = false;
	bool FetchSafetyChecks;
};

export class System : public SystemBase
{
	friend class SystemManager;

public:
	System() {}
	~System() {}

protected:

	virtual void UpdatePerEntity(float deltaTime, Entity entity, EntityType entityType) = 0;


	template <typename ComponentType>
	void AddWantedComponentDataForReading(ReadOnlyComponentStorage<ComponentType>* storage)
	{
		AddReadOnlyComponentFetcher(&storage->Storage);
	}

	template <typename ComponentType>
	void AddWantedComponentDataForWriting(WriteReadComponentStorage<ComponentType>* storage)
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
	void AddAlwaysFetchedEntitiesForReading(EntityType typeID, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		AddReadOnlyAlwaysFetch(typeID, &storage->Storage);
	}

	template <typename ComponentType>
	void AddAlwaysFetchedEntitiesForWriting(EntityType typeID, WriteReadAlwaysFetchedStorage<ComponentType>* storage)
	{
		AddWriteReadAlwaysFetch(typeID, &storage->Storage);
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
		size_t index = WorldEntityManager->GetEntityStorageID(entity);
		return storage->Storage[index];
	}

	template <typename ComponentType>
	ComponentType GetEntityComponent(Entity entity, EntityType entityType, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		size_t index = WorldEntityManager->GetEntityStorageID(entity, entityType);
		return storage->Storage[index];
	}

	template <typename ComponentType>
	ComponentType& GetEntityComponent(Entity entity, WriteReadAlwaysFetchedStorage<ComponentType>* storage)
	{
		size_t index = WorldEntityManager->GetEntityStorageID(entity);
		return storage->Storage[index];
	}

	template <typename ComponentType>
	ComponentType& GetEntityComponent(Entity entity, EntityType entityType, WriteReadAlwaysFetchedStorage<ComponentType>* storage)
	{
		size_t index = WorldEntityManager->GetEntityStorageID(entity, entityType);
		return storage->Storage[index];
	}

	/// <summary>
	/// Executes a given function with every entity the system handles. WARNING: overrides the interface like GetCurrentEntityComponent()
	/// to fetch components from entities processed in this loop.
	/// </summary>
	/// <param name="function"></param>
	void ExecutePerEntity(std::function<void(float, Entity, EntityType)> function)
	{
		for (size_t vectorIndex = 0; vectorIndex < FetchedEntitiesInfo.size(); vectorIndex++)
		{
			auto info = FetchedEntitiesInfo[vectorIndex];
			for (size_t spanIndex = 0; spanIndex < info.EntityLocators.size(); spanIndex++)
			{
				auto locator = info.EntityLocators[spanIndex];
				RuntimeThreadInfo.outerIndex = vectorIndex;
				RuntimeThreadInfo.innerIndexRead = spanIndex;
				RuntimeThreadInfo.innerIndexWrite = spanIndex;
				std::invoke(function, DeltaTime, locator.ID, info.Type);
			}
		}
	}


private:

	virtual void SystemUpdate(float deltaTime)
	{
		DeltaTime = deltaTime;
		UpdateMatchingEntityTypes();
		FetchRequiredSpans();
		FetchedEntitiesInfo = WorldEntityManager->GetEntityFetchInfo(MatchingEntityTypes);
		Update(deltaTime);
		if (!DisableAutomaticEntityLoop)
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
					UpdatePerEntity(deltaTime, locator.ID, info.Type);
				}
			}
		}
	}

protected:
	bool DisableAutomaticEntityLoop = false;

private:
	float DeltaTime;
	ThreadInfo RuntimeThreadInfo;
};



struct DataOffsets
{
	size_t outerIndex = 0;
	size_t startOffset = 0;
	size_t endOffset = 0;
};

export class ThreadedSystem : public SystemBase
{
	friend class SystemManager;

public:
	ThreadedSystem() {}
	~ThreadedSystem() {}


	virtual void Init() {}
	virtual void PreUpdate(float deltaTime) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void PostUpdate(float deltaTime) = 0;
	virtual void Destroy() = 0;
	virtual void UpdatePerEntityThreaded(int thread, float deltaTime, Entity entity, EntityType entityType) = 0;

	template <typename ComponentType>
	void AddWantedComponentDataForReading(ReadOnlyComponentStorage<ComponentType>* storage)
	{
		AddReadOnlyComponentFetcher(&storage->Storage);
	}

	template <typename ComponentType>
	void AddWantedComponentDataForWriting(WriteReadComponentStorage<ComponentType>* writeReadStorage, ThreadComponentStorage<ComponentType>* threadStorage)
	{
		AddWriteReadComponentFetcher(&writeReadStorage->Storage);

		threadStorage->Storage = std::vector<std::vector<std::vector<ComponentType>>>(ThreadCount);
		for (size_t thread = 0; thread < ThreadCount; thread++)
		{
			AddThreadDataCopierForWriting(thread,
				&writeReadStorage->Storage,
				&threadStorage->Storage[thread]);
		}
	}

	template <typename ComponentType>
	void AddSystemDataForReading(ReadOnlySystemDataStorage<ComponentType>* storage)
	{
		AddReadOnlySystemDataFetcher(&storage->Storage);
	}

	template <typename ComponentType>
	void AddAlwaysFetchedEntitiesForReading(EntityType typeID, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		AddReadOnlyAlwaysFetch(typeID, &storage->Storage);
	}

	template <typename ComponentType>
	ComponentType GetCurrentEntityComponent(int thread, ReadOnlyComponentStorage<ComponentType>* storage)
	{
		auto& info = RuntimeThreadInfo[thread];
		return storage->Storage[info.outerIndex][info.innerIndexRead];
	}

	template <typename ComponentType>
	ComponentType& GetCurrentEntityComponent(int thread, ThreadComponentStorage<ComponentType>* storage)
	{
		auto& info = RuntimeThreadInfo[thread];
		return storage->Storage[thread][info.outerIndex][info.innerIndexWrite];
	}

	template <typename ComponentType>
	ComponentType GetSystemData(ReadOnlySystemDataStorage<ComponentType>* storage)
	{
		return storage->Storage[0];
	}

	template <typename ComponentType>
	ComponentType GetEntityComponent(Entity entity, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		size_t index = WorldEntityManager->GetEntityStorageID(entity);
		return storage->Storage[index];
	}

	template <typename ComponentType>
	ComponentType GetEntityComponent(Entity entity, EntityType entityType, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		size_t index = WorldEntityManager->GetEntityStorageID(entity, entityType);
		return storage->Storage[index];
	}

protected:
	/// <summary>
	/// Executes a given function with every entity the system handles. WARNING: overrides the interface like GetCurrentEntityComponent()
	/// to fetch components from entities processed in this loop.
	/// </summary>
	/// <param name="function"></param>
	void ExecutePerEntityThreaded(std::function<void(int, float, Entity, EntityType)> function)
	{
		CopyDataForThreads();
		for (int thread = 0; thread < ThreadCount; thread++)
		{
			const auto& threadOffsets = ThreadDataOffsets[thread];
			if (ThreadEntityInfo[thread].size() > 0)
			{
				ThreadRunner->AddTask(std::bind_front(&ThreadedSystem::ThreadCustomTask, this), function, thread, CurrentDeltaTime, std::as_const(threadOffsets), std::as_const(ThreadEntityInfo[thread]));
			}
		}
		ThreadRunner->RunAllTasks();
		SaveThreadsData();
	}


private:
	virtual void SystemUpdate(float deltaTime) override
	{
		CurrentDeltaTime = deltaTime;
		UpdateMatchingEntityTypes();
		FetchedEntitiesInfo = WorldEntityManager->GetEntityFetchInfo(MatchingEntityTypes);
		CalculateThreadAccessOffsets();
		FetchRequiredSpans();
		Update(deltaTime);

		if (!DisableAutomaticEntityLoop)
		{
			CopyDataForThreads();
			for (int thread = 0; thread < ThreadCount; thread++)
			{
				const auto& threadOffsets = ThreadDataOffsets[thread];
				//ThreadTask(thread, deltaTime, threadOffsets, FetchedEntitiesInfo);
				if (ThreadEntityInfo[thread].size() > 0)
				{
					ThreadRunner->AddTask(std::bind_front(&ThreadedSystem::ThreadTask, this), thread, deltaTime, std::as_const(threadOffsets), std::as_const(ThreadEntityInfo[thread]));
				}
			}
			ThreadRunner->RunAllTasks();
			SaveThreadsData();
		}

	}

	void ThreadTask(int thread, float deltaTime, const std::vector<DataOffsets>& threadOffsets, const std::vector<EntitySpanFetchInfo>& entitiesInfo)
	{
		auto& currentThreadInfo = RuntimeThreadInfo[thread];
		for (size_t i = 0; i < threadOffsets.size(); i++)
		{
			const auto& offset = threadOffsets[i];
			const EntitySpanFetchInfo& entityInfos = entitiesInfo[i];
			for (size_t innerIndex = 0; innerIndex < offset.endOffset - offset.startOffset; innerIndex++)
			{
				const EntityStorageLocator& locator = entityInfos.EntityLocators[innerIndex];
				currentThreadInfo.outerIndex = offset.outerIndex;
				currentThreadInfo.innerIndexWrite = innerIndex;
				currentThreadInfo.innerIndexRead = innerIndex + offset.startOffset;
				UpdatePerEntityThreaded(thread, deltaTime, locator.ID, entityInfos.Type);
			}
		}
	}

	void ThreadCustomTask(std::function<void(int, float, Entity, EntityType)> function, int thread, float deltaTime, const std::vector<DataOffsets>& threadOffsets, const std::vector<EntitySpanFetchInfo>& entitiesInfo)
	{
		auto& currentThreadInfo = RuntimeThreadInfo[thread];
		for (size_t i = 0; i < threadOffsets.size(); i++)
		{
			const auto& offset = threadOffsets[i];
			const EntitySpanFetchInfo& entityInfos = entitiesInfo[i];
			for (size_t innerIndex = 0; innerIndex < offset.endOffset - offset.startOffset; innerIndex++)
			{
				const EntityStorageLocator& locator = entityInfos.EntityLocators[innerIndex];
				currentThreadInfo.outerIndex = offset.outerIndex;
				currentThreadInfo.innerIndexWrite = innerIndex;
				currentThreadInfo.innerIndexRead = innerIndex + offset.startOffset;
				std::invoke(function, thread, deltaTime, locator.ID, entityInfos.Type);
			}
		}
	}

	template <typename ComponentType>
	void AddThreadDataCopierForWriting(int thread,
		std::vector<std::span<ComponentType>>* storage,
		std::vector<std::vector<ComponentType>>* threadStorage)
	{

		auto copier = [=]() {
			ThreadDataCopier(thread, std::as_const(storage), threadStorage);
			};
		ThreadCopyFunctions.push_back(copier);
		auto saver = [=]() {
			ThreadDataSaver(thread, storage, threadStorage);
			};
		ThreadSaveFunctions.push_back(saver);
	}

	template <typename ComponentType>
	void ThreadDataCopier(int thread,
		const std::vector<std::span<ComponentType>>* componentSpans,
		std::vector<std::vector<ComponentType>>* threadStorages)
	{
		threadStorages->clear();
		const std::vector<DataOffsets>& threadOffsets = ThreadDataOffsets[thread];
		if (threadOffsets.size() == 0) { return; }
		
		*threadStorages = std::vector<std::vector<ComponentType>>(componentSpans->size());
		for (size_t i = 0; i < threadOffsets.size(); i++)
		{
			const auto& offset = threadOffsets[i];
			std::vector<ComponentType>& storage = (*threadStorages)[offset.outerIndex];
			const std::span<ComponentType>& span = (*componentSpans)[offset.outerIndex];
			storage.clear();
			storage = std::vector<ComponentType>(offset.endOffset - offset.startOffset);
			auto begin = span.begin() + offset.startOffset;
			auto end = span.begin() + offset.endOffset;
			std::ranges::copy(begin, end, storage.begin());
		}
	}

	template <typename ComponentType>
	void ThreadDataSaver(int thread,
		std::vector<std::span<ComponentType>>* componentSpans,
		std::vector<std::vector<ComponentType>>* threadStorages)
	{
		const std::vector<DataOffsets>& threadOffsets = ThreadDataOffsets[thread];
		if (threadOffsets.size() == 0) { return; }
		for (size_t i = 0; i < threadOffsets.size(); i++)
		{
			const auto& offset = threadOffsets[i];
			std::span<ComponentType>& span = (*componentSpans)[offset.outerIndex];
			const std::vector<ComponentType>& storage = (*threadStorages)[offset.outerIndex];
			auto begin = storage.begin();
			auto end = storage.end();
			auto spanIt = span.begin() + offset.startOffset;
			std::ranges::copy(storage, spanIt);
			auto begin2 = spanIt;

		}
	}

	std::vector<DataOffsets> GetThreadAccessOffsets(int thread)
	{
		return ThreadDataOffsets[thread];
	}

	void CalculateThreadAccessOffsets()
	{
		ThreadDataOffsets = std::vector<std::vector<DataOffsets>>(ThreadCount, std::vector<DataOffsets>());
		ThreadEntityInfo = std::vector<std::vector<EntitySpanFetchInfo>>(ThreadCount, std::vector<EntitySpanFetchInfo>());

		size_t entityCount = 0;
		for (auto& info : FetchedEntitiesInfo)
		{
			entityCount += info.EntityCount;
		}

		if (entityCount / ThreadCount < MinEntitiesPerThread)
		{
			AllDataToFirstThread();
			return;
		}

		size_t entitiesPerThread = std::floor(entityCount / ThreadCount);
		size_t extraToFirstThread = entityCount % ThreadCount;

		int currentThread = 1;
		int currentOuterIndex;
		size_t currentThreadEntitiesLeft = entitiesPerThread + extraToFirstThread;
		size_t transferEntitiesCount = 0;
		size_t entitiesTransferred = 0;
		size_t entitiesLeftToTransfer = entityCount;
		for (size_t currentFetchEntity = 0; currentFetchEntity < FetchedEntitiesInfo.size(); currentFetchEntity++)
		{
			size_t currentStartOffset = 0;
			size_t currentEndOffset = 0;
			size_t transferredEntitiesCount = 0;
			size_t typeEntityCount = FetchedEntitiesInfo[currentFetchEntity].EntityCount;
			size_t typeEntitiesLeftToTransfer = typeEntityCount;
			if (typeEntityCount == 0)
			{
				continue;
			}
			for (size_t i = 0; i < ThreadCount; i++)
			{
				if (currentThreadEntitiesLeft >= typeEntitiesLeftToTransfer)
				{
					transferEntitiesCount = typeEntitiesLeftToTransfer;
				}
				else
				{
					transferEntitiesCount = currentThreadEntitiesLeft;
				}
				currentEndOffset = currentStartOffset + transferEntitiesCount;
				transferredEntitiesCount += transferEntitiesCount;
				entitiesTransferred += transferEntitiesCount;
				currentThreadEntitiesLeft -= transferEntitiesCount;
				entitiesLeftToTransfer -= transferEntitiesCount;
				typeEntitiesLeftToTransfer -= transferEntitiesCount;

				DataOffsets offsets
				{
					.outerIndex = currentFetchEntity,
					.startOffset = currentStartOffset,
					.endOffset = currentEndOffset
				};
				// Copy Entity info for thread
				EntitySpanFetchInfo entityInfo;
				entityInfo.EntityCount = offsets.endOffset - offsets.startOffset;
				entityInfo.EntityLocators = std::vector<EntityStorageLocator>(entityInfo.EntityCount);
				auto begin = FetchedEntitiesInfo[currentFetchEntity].EntityLocators.begin() + currentStartOffset;
				auto end = FetchedEntitiesInfo[currentFetchEntity].EntityLocators.begin() + currentEndOffset;
				auto destinationIt = entityInfo.EntityLocators.begin();
				std::ranges::copy(begin, end, destinationIt);
				entityInfo.Type = FetchedEntitiesInfo[currentFetchEntity].Type;
				entityInfo.TypeName = FetchedEntitiesInfo[currentFetchEntity].TypeName;

				currentStartOffset = currentEndOffset;

				if (currentThreadEntitiesLeft == 0)
				{
					ThreadDataOffsets[currentThread - 1].push_back(offsets);
					ThreadEntityInfo[currentThread - 1].push_back(entityInfo);
					currentThread++;
					currentThreadEntitiesLeft = entitiesPerThread;
					if (entitiesLeftToTransfer == 0)
					{
						break;
					}
				}
				else if (transferredEntitiesCount == typeEntityCount)
				{
					ThreadDataOffsets[currentThread - 1].push_back(offsets);
					ThreadEntityInfo[currentThread - 1].push_back(entityInfo);
					break;
				}
			}
			if (entitiesLeftToTransfer == 0)
			{
				break;
			}
		}

		if (entitiesTransferred != entityCount)
		{
			fmt::println("ERROR: When assigning entities to threads, all of the entities were not allocated!");
			abort();
		}
	}

	void AllDataToFirstThread()
	{
		for (size_t currentFetchEntity = 0; currentFetchEntity < FetchedEntitiesInfo.size(); currentFetchEntity++)
		{
			size_t entityCount = FetchedEntitiesInfo[currentFetchEntity].EntityCount;
			DataOffsets offsets =
				{
					.outerIndex = currentFetchEntity,
					.startOffset = 0,
					.endOffset = entityCount
				};

			ThreadDataOffsets[0].push_back(offsets);
			EntitySpanFetchInfo entityInfo = FetchedEntitiesInfo[currentFetchEntity];
			ThreadEntityInfo[0].push_back(entityInfo);
		}
		for (size_t i = 1; i < ThreadDataOffsets.size(); i++)
		{
			ThreadDataOffsets[i] = std::vector<DataOffsets>();
			ThreadEntityInfo[i] = std::vector<EntitySpanFetchInfo>();
		}
	}

	void CopyDataForThreads()
	{
		for (auto& copier : ThreadCopyFunctions)
		{
			std::invoke(copier);
		}
	}

	void SaveThreadsData()
	{
		for (auto& saver : ThreadSaveFunctions)
		{
			std::invoke(saver);
		}
	}


protected:
	int GetThreadCount() { return ThreadCount; }
	bool DisableAutomaticEntityLoop = false;

private:
	std::vector<ThreadInfo> RuntimeThreadInfo;
	ThreadedTaskRunner* ThreadRunner;
	int MinEntitiesPerThread = 8;
	int ThreadCount = 1;
	float CurrentDeltaTime;

	std::vector<std::vector<DataOffsets>> ThreadDataOffsets;
	std::vector<std::vector<EntitySpanFetchInfo>> ThreadEntityInfo;
	std::vector<std::function<void()>> ThreadCopyFunctions;
	std::vector<std::function<void()>> ThreadSaveFunctions;
};

export
template <typename T>
concept SystemBaseClass = std::derived_from<T, SystemBase>;

export
template <typename T>
concept SystemClass = std::derived_from<T, System>;

export
template <typename T>
concept ThreadedSystemClass = std::derived_from<T, ThreadedSystem>;
