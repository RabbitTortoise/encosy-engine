module;
#include <fmt/core.h>
#include <array>

export module EncosyCore.SystemThreaded;

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
import <utility>;


struct DataOffsets
{
	size_t outerIndex = 0;
	size_t startOffset = 0;
	size_t endOffset = 0;
};

export struct ThreadEntityAccessInfo
{
	EntityType Type;
	std::string TypeName;
	size_t EntityCount;
	std::vector<EntityStorageLocator> EntityLocators;
};

export
template <typename ComponentType>
struct ThreadComponentStorage
{
	std::vector<std::vector<std::vector<ComponentType>>> Storage;
};

export struct SystemThreadedOptions
{
	bool PreferRunAlone = false;
	bool ThreadedUpdateCalls = false;
	bool AllowPotentiallyUnsafeEdits = false;
	bool AllowDestructiveEditsInThreads = false;
	bool IgnoreThreadSaveFunctions = false;
};

export class SystemThreaded : public SystemBase
{
	friend class SystemManager;

public:
	SystemThreaded()
	{
		RunAlone = false;
	}
	~SystemThreaded() override
	{
	}

protected:

	//virtual void Init() {} //SystemBase
	virtual void PreUpdate(int thread, const double deltaTime) = 0;
	virtual void Update(int thread, const double deltaTime) = 0;
	virtual void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) = 0;
	virtual void PostUpdate(int thread, const double deltaTime) = 0;
	//virtual void Destroy() = 0; //SystemBase

	void SetThreadedRunOptions(SystemThreadedOptions options)
	{
		RunAlone = options.PreferRunAlone;
		ThreadedUpdateCalls = options.ThreadedUpdateCalls;
		IgnoreThreadSaveFunctions = options.IgnoreThreadSaveFunctions;
		AllowDestructiveEditsInThreads = options.AllowDestructiveEditsInThreads;
		AllowPotentiallyUnsafeEdits = options.AllowPotentiallyUnsafeEdits;
		if (IgnoreThreadSaveFunctions && AllowDestructiveEditsInThreads || AllowPotentiallyUnsafeEdits)
		{
			RuntimeModificationsAllowed = true;
		}
	}

	template <typename ComponentType>
	void AddComponentQueryForReading(ReadOnlyComponentStorage<ComponentType>* storage)
	{
		AddReadOnlyComponentFetcher(&storage->Storage);
	}

	template <typename ComponentType>
	void AddComponentQueryForWriting(WriteReadComponentStorage<ComponentType>* writeReadStorage, ThreadComponentStorage<ComponentType>* threadStorage)
	{
		AddWriteReadComponentFetcher(&writeReadStorage->Storage);
		AddThreadDataCopierForWriting(writeReadStorage, threadStorage);
	}

	template <typename ComponentType>
	void AddSystemDataForReading(ReadOnlySystemDataStorage<ComponentType>* storage)
	{
		AddReadOnlySystemDataFetcher(&storage->Storage);
	}

	template <typename ComponentType>
	void AddEntitiesForReading(EntityType typeID, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		AddReadOnlyAlwaysFetchEntityType(typeID, &storage->Storage);
	}

	template <typename ComponentType>
	void AddComponentsForReading(ReadOnlyComponentStorage<ComponentType>* storage)
	{
		AddReadOnlyAlwaysFetchComponents(&storage->Storage);
	}

	template <typename ComponentType>
	ComponentType GetCurrentEntityComponent(const int thread, ReadOnlyComponentStorage<ComponentType>* storage)
	{
		const auto& info = RuntimeThreadInfo[thread];
		return storage->Storage[info.outerIndex][info.innerIndexRead];
	}

	template <typename ComponentType>
	ComponentType& GetCurrentEntityComponent(const int thread, ThreadComponentStorage<ComponentType>* storage)
	{
		const auto& info = RuntimeThreadInfo[thread];
		return storage->Storage[thread][info.outerIndex][info.innerIndexWrite];
	}

	template <typename ComponentType>
	ComponentType GetSystemData(ReadOnlySystemDataStorage<ComponentType>* storage)
	{
		return storage->Storage[0];
	}

	template <typename ComponentType>
	ComponentType GetEntityComponent(Entity entity, EntityType entityType, ReadOnlyAlwaysFetchedStorage<ComponentType>* storage)
	{
		const size_t index = WorldEntityManager->GetEntityComponentIndex(entity, entityType);
		return storage->Storage[index];
	}

	void AddCustomSaveFunction(std::function<void()>&& saveFunction)
	{
		ThreadSaveFunctions.emplace_back(saveFunction);
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromEntity(const Entity entity)
	{
		return WorldEntityManager->GetReadOnlyComponentFromEntityThreaded<ComponentType>(entity);
	}

	template<typename ComponentType>
	bool HasComponentType(const Entity entity)
	{
		return WorldEntityManager->HasComponentTypeThreaded<ComponentType>(entity);
	}

	EntityOperationResult GetEntityTypeInfo(const std::string& entityTypeName)
	{
		return WorldEntityManager->GetEntityTypeInfo(entityTypeName);
	}

	template<typename...ComponentTypes>
	Entity CreateEntityWithData(const EntityType entityType, ComponentTypes&&... components)
	{
		if (!CheckRuntimeModificationsAllowed()) 
		{ 
			fmt::println("ERROR: Can't use CreateEntityWithData with current system options.");
			return -1; 
		}
		if (!DestructiveEntityStorageAccess.contains(entityType))
		{
			fmt::println("ERROR: CreateEntityWithDataThreaded(): System does not have destructive access to entity type {}", entityType);
			return false;
		}
		
		return WorldEntityManager->CreateEntityWithDataThreaded(entityType, std::forward<decltype(std::move(components))>(std::move(components))...);
	}
	
	bool DeleteEntity(const Entity entity, const EntityType entityType)
	{
		if (!CheckRuntimeModificationsAllowed())
		{
			fmt::println("ERROR: Can't use DeleteEntity with current system options."); 
			return false;
		}

		if (!DestructiveEntityStorageAccess.contains(entityType))
		{
			fmt::println("ERROR: DeleteEntityThreaded(): System does have destructive acccess to entity type {}", entity, entityType);
			return false;
		}

		return WorldEntityManager->DeleteEntityThreaded(entity, entityType);
	}

	template<typename... DeleteComponentTypes, typename... NewComponentTypes>
	bool ModifyEntityComponents(const Entity entity, const EntityType entityType, const EntityType newEntityType, NewComponentTypes&&... newComponents)
	{
		if (!CheckRuntimeModificationsAllowed())
		{
			fmt::println("ERROR: Can't use DeleteEntity with current system options.");  
			return false; 
		}
		if (!DestructiveEntityStorageAccess.contains(entityType))
		{
			fmt::println("ERROR: ModifyEntityComponentsThreaded(): System doen not have access to modify entity type {}", entityType);
			return false;
		}
		if (!DestructiveEntityStorageAccess.contains(newEntityType))
		{
			fmt::println("ERROR: ModifyEntityComponentsThreaded(): System doen not have access to modify entity type {}", newEntityType);
			return false;
		}
		const auto success = WorldEntityManager->ModifyEntityComponentsThreaded<DeleteComponentTypes...>(entity, entityType, newEntityType, std::forward<decltype(std::move(newComponents))>(std::move(newComponents))...);
		return success;
	}

	template<typename ComponentType>
	void CreateNewComponentToStorage(const ComponentStorageLocator& locator, ComponentType&& component)
	{
		if (!CheckRuntimeModificationsAllowed())
		{
			fmt::println("ERROR: Can't use CreateNewComponentToStorage with current system options."); 
			return; 
		}
		WorldComponentManager->CreateNewComponentToStorageThreaded(locator, std::forward<decltype(std::move(component))>(std::move(component)));
	}

	void ResetComponentStorage(const ComponentStorageLocator& locator) const
	{
		if (!CheckRuntimeModificationsAllowed())
		{
			fmt::println("ERROR: Can't use ResetComponentStorage with current system options.");
			return;
		}
		WorldComponentManager->ResetComponentStorageThreaded(locator);
	}

private:

	bool CheckRuntimeModificationsAllowed() const
	{
		if (!RuntimeModificationsAllowed)
		{
			bool allowed = true;
			if (!IgnoreThreadSaveFunctions)
			{
				fmt::println("ERROR: IgnoreThreadSaveFunctions option is set to false!");
				allowed = false;
			}
			if (!AllowDestructiveEditsInThreads)
			{
				fmt::println("ERROR: AllowDestructiveEditsInThreads option is set to false!");
				allowed = false;
			}
			return allowed;
		}
		return true;
	}

	void SystemInit() override
	{
		ThreadDataOffsets = std::vector(ThreadCount, std::vector<DataOffsets>());
		ThreadEntityInfo = std::vector(ThreadCount, std::vector<ThreadEntityAccessInfo>());
		ThreadCopyFunctions = std::vector(ThreadCount, std::vector<std::function<void()>>());
	}
	 
	void SystemPreUpdate(const double deltaTime) override
	{
		CurrentDeltaTime = deltaTime;
		if (ThreadedUpdateCalls)
		{
			for (int thread = 0; thread < ThreadCount; thread++)
			{
				ThreadRunner->AddWorkTask(std::bind_front(&SystemThreaded::PreUpdate, this), thread, CurrentDeltaTime);
			}
			return;
		}
		else if (RunAlone)
		{
			PreUpdate(0, CurrentDeltaTime);
			return;
		}
		ThreadRunner->AddWorkTask(std::bind_front(&SystemThreaded::PreUpdate, this), 0, CurrentDeltaTime);
	}

	void SystemUpdate() override
	{
		UpdateMatchingEntityTypes();
		FetchedEntitiesInfo = WorldEntityManager->GetEntityFetchInfo(MatchingEntityTypes);
		CalculateThreadAccessOffsets();
		FetchRequiredSpans();

		if (ThreadedUpdateCalls)
		{
			for (int thread = 0; thread < ThreadCount; thread++)
			{
				ThreadRunner->AddWorkTask(std::bind_front(&SystemThreaded::Update, this), thread, CurrentDeltaTime);
			}
			return;
		}
		else if (RunAlone)
		{
			Update(0, CurrentDeltaTime);
			return;
		}
		ThreadRunner->AddWorkTask(std::bind_front(&SystemThreaded::Update, this), 0, CurrentDeltaTime);
	}

	void SystemPerEntityUpdate() override
	{
		for (int thread = 0; thread < ThreadCount; thread++)
		{
			for (const auto& copyFunction : ThreadCopyFunctions[thread])
			{
				ThreadPerEntityRunner->AddCopyTask(copyFunction);
			}

			const auto& threadOffsets = ThreadDataOffsets[thread];
			if (ThreadEntityInfo[thread].size() > 0)
			{
				ThreadPerEntityRunner->AddWorkTask(std::bind_front(&SystemThreaded::ThreadPerEntityTask, this), thread, CurrentDeltaTime, std::as_const(threadOffsets), std::as_const(ThreadEntityInfo[thread]));
			}
		}
		if (!IgnoreThreadSaveFunctions)
		{
			for (const auto& threadSaveFunction : ThreadSaveFunctions)
			{
				ThreadPerEntityRunner->AddSaveTask(threadSaveFunction);
			}
		}
	}

	void SystemPostUpdate() override
	{
		if (ThreadedUpdateCalls)
		{
			for (int thread = 0; thread < ThreadCount; thread++)
			{
				ThreadRunner->AddWorkTask(std::bind_front(&SystemThreaded::PostUpdate, this), thread, CurrentDeltaTime);
			}
			return;
		}
		else if (RunAlone)
		{
			PostUpdate(0, CurrentDeltaTime);
			return;
		}
		ThreadRunner->AddWorkTask(std::bind_front(&SystemThreaded::PostUpdate, this), 0, CurrentDeltaTime);
	}

	void ThreadPerEntityTask(int thread, const double deltaTime, const std::vector<DataOffsets>& threadOffsets, const std::vector<ThreadEntityAccessInfo>& entitiesInfo)
	{
		auto& currentThreadInfo = RuntimeThreadInfo[thread];
		for (size_t i = 0; i < threadOffsets.size(); i++)
		{
			const auto& offset = threadOffsets[i];
			const ThreadEntityAccessInfo& entityInfos = entitiesInfo[i];
			for (size_t innerIndex = 0; innerIndex < offset.endOffset - offset.startOffset; innerIndex++)
			{
				const EntityStorageLocator& locator = entityInfos.EntityLocators[innerIndex];
				currentThreadInfo.outerIndex = offset.outerIndex;
				currentThreadInfo.innerIndexWrite = innerIndex;
				currentThreadInfo.innerIndexRead = innerIndex + offset.startOffset;
				UpdatePerEntity(thread, deltaTime, locator.Entity, entityInfos.Type);
			}
		}
	}

	template <typename ComponentType>
	void AddThreadDataCopierForWriting(
		WriteReadComponentStorage<ComponentType>* writeReadStorage,
		ThreadComponentStorage<ComponentType>* threadCopyStorage)
	{
		threadCopyStorage->Storage = std::vector<std::vector<std::vector<ComponentType>>>(ThreadCount);


		auto* storage = &writeReadStorage->Storage;
		for (size_t thread = 0; thread < ThreadCount; thread++)
		{
			auto* threadStorage = &threadCopyStorage->Storage[thread];
			auto copier = [=]() {
				ThreadDataCopier(thread, std::as_const(storage), threadStorage);
				};
			ThreadCopyFunctions[thread].emplace_back(std::move(copier));
		}


		auto saver = [=]() {
			auto* storage = &writeReadStorage->Storage;
			for (size_t thread = 0; thread < ThreadCount; thread++)
			{
				auto* threadStorage = &threadCopyStorage->Storage[thread];
				ThreadDataSaver(thread, std::as_const(storage), threadStorage);
			}
			};
		ThreadSaveFunctions.push_back(saver);
	}

	template <typename ComponentType>
	void ThreadDataCopier(int thread,
		const std::vector<std::span<ComponentType>>* componentSpans,
		std::vector<std::vector<ComponentType>>* threadStorages)
	{
		const std::vector<DataOffsets>& threadOffsets = ThreadDataOffsets[thread];
		if (threadOffsets.size() == 0) { return; }

		for (auto& storage : (*threadStorages))
		{
			storage.clear();
		}
		(*threadStorages).resize(componentSpans->size());
		for (size_t i = 0; i < threadOffsets.size(); i++)
		{
			const auto& offset = threadOffsets[i];
			std::vector<ComponentType>& storage = (*threadStorages)[offset.outerIndex];
			const std::span<ComponentType>& span = (*componentSpans)[offset.outerIndex];
			storage.resize(offset.endOffset - offset.startOffset);
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
			std::ranges::move(storage, spanIt);
		}
	}

	std::vector<DataOffsets> GetThreadAccessOffsets(const int thread)
	{
		return ThreadDataOffsets[thread];
	}

	void CalculateThreadAccessOffsets()
	{
		for (auto& threadDataOffset : ThreadDataOffsets)
		{
			threadDataOffset.clear();
		}
		for (auto& threadEntityInfo : ThreadEntityInfo)
		{
			threadEntityInfo.clear();
		}

		size_t entityCount = 0;
		for (const auto& info : FetchedEntitiesInfo)
		{
			entityCount += info.EntityCount;
		}

		if (entityCount / ThreadCount < MinEntitiesPerThread)
		{
			AllDataToFirstThread();
			return;
		}

		const size_t entitiesPerThread = std::floor(entityCount / ThreadCount);
		size_t extra = entityCount % ThreadCount;

		int currentThread = 0;
		size_t currentThreadEntitiesLeft = entitiesPerThread;
		if(extra > 0)
		{
			currentThreadEntitiesLeft++;
			extra -= 1;
		}
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
			for (size_t i = currentThread; i < ThreadCount; i++)
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
				ThreadEntityAccessInfo entityInfo;
				entityInfo.EntityCount = offsets.endOffset - offsets.startOffset;
				entityInfo.EntityLocators = std::vector<EntityStorageLocator>(entityInfo.EntityCount);
				const auto begin = FetchedEntitiesInfo[currentFetchEntity].EntityLocators.begin() + currentStartOffset;
				const auto end = FetchedEntitiesInfo[currentFetchEntity].EntityLocators.begin() + currentEndOffset;
				const auto destinationIt = entityInfo.EntityLocators.begin();
				std::ranges::copy(begin, end, destinationIt);
				entityInfo.Type = FetchedEntitiesInfo[currentFetchEntity].Type;
				entityInfo.TypeName = FetchedEntitiesInfo[currentFetchEntity].TypeName;

				currentStartOffset = currentEndOffset;

				if (currentThreadEntitiesLeft == 0)
				{
					ThreadDataOffsets[currentThread].emplace_back(offsets);
					ThreadEntityInfo[currentThread].emplace_back(entityInfo);
					currentThread++;
					currentThreadEntitiesLeft = entitiesPerThread;
					if (extra > 0)
					{
						currentThreadEntitiesLeft++;
						extra -= 1;
					}
					if (entitiesLeftToTransfer == 0)
					{
						break;
					}
				}
				else if (transferredEntitiesCount == typeEntityCount)
				{
					ThreadDataOffsets[currentThread].emplace_back(offsets);
					ThreadEntityInfo[currentThread].emplace_back(entityInfo);
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
			const size_t entityCount = FetchedEntitiesInfo[currentFetchEntity].EntityCount;
			DataOffsets offsets =
			{
				.outerIndex = currentFetchEntity,
				.startOffset = 0,
				.endOffset = entityCount
			};

			ThreadDataOffsets[0].push_back(offsets);
			const auto& fetchedEntityInfo = FetchedEntitiesInfo[currentFetchEntity];
			ThreadEntityAccessInfo accessInfo;
			accessInfo.Type = fetchedEntityInfo.Type;
			accessInfo.TypeName = fetchedEntityInfo.TypeName;
			accessInfo.EntityCount = fetchedEntityInfo.EntityCount;
			std::ranges::copy(fetchedEntityInfo.EntityLocators, std::back_inserter(accessInfo.EntityLocators));

			ThreadEntityInfo[0].push_back(accessInfo);
		}
		for (size_t i = 1; i < ThreadDataOffsets.size(); i++)
		{
			ThreadDataOffsets[i] = std::vector<DataOffsets>();
			ThreadEntityInfo[i] = std::vector<ThreadEntityAccessInfo>();
		}
	}

protected:
	int GetThreadCount() const { return ThreadCount; }

private:
	ThreadedTaskRunner* ThreadRunner = nullptr;
	ThreadedPerEntityTaskRunner* ThreadPerEntityRunner = nullptr;
	std::vector<SystemThreadInfo> RuntimeThreadInfo;
	int MinEntitiesPerThread = 1;
	int ThreadCount = 1;

	std::vector<std::vector<DataOffsets>> ThreadDataOffsets;
	std::vector<std::vector<ThreadEntityAccessInfo>> ThreadEntityInfo;
	std::vector<std::vector<std::function<void()>>> ThreadCopyFunctions;
	std::vector<std::function<void()>> ThreadSaveFunctions;
	bool ThreadedUpdateCalls = false;
	bool IgnoreThreadSaveFunctions = false;
	bool AllowDestructiveEditsInThreads = false;
	bool RuntimeModificationsAllowed = false;
	bool AllowPotentiallyUnsafeEdits = false;
};

export
template <typename T>
concept SystemThreadedClass = std::derived_from<T, SystemThreaded>;
