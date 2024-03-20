module;
#include <fmt/core.h>
#include <array>

export module EncosyCore.SystemBase;
import EncosyCore.Entity;
import EncosyCore.ComponentManager;
import EncosyCore.ComponentTypeStorage;
import EncosyCore.EntityManager;
import EncosyCore.EntityTypeStorage;
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


export typedef size_t SystemID;
export enum class SystemType { System = 0, RenderSystem, PhysicsSystem };

export enum class SystemSyncPoint
{
	First = 0,
	BeforeEngineSystems,
	WithEngineSystems,
	AfterEngineSystems,
	Last
};

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

export struct SystemEntityFetchInfo
{
	EntitySpanFetchInfo fetchInfo;
	std::type_index componentType;
};

export struct SystemThreadInfo
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
	friend class SystemThreaded;

public:
	SystemBase() {}
	~SystemBase() {}

	SystemType GetType() const { return Type; }
	SystemID GetID() const { return ID; }
	bool GetInitialized() const { return bInitialized; }
	bool GetEnabled() const { return bEnabled; }
	bool GetRunAlone() { return RunAlone; }

protected:

	virtual void Init() = 0;
	virtual void Destroy() = 0;

	template <typename ComponentType>
	void AddForbiddenComponentQuery()
	{
		auto& id = typeid(ComponentType);
		ForbiddenComponentTypes.insert(id);
	}

	void EnableDestructiveAccessToEntityStorage(const EntityType entityType)
	{
		DestructiveEntityStorageAccess.insert(entityType);
	}

private:
	virtual void SystemInit() = 0;
	virtual void SystemPreUpdate(const double deltaTime) = 0;
	virtual void SystemUpdate() = 0;
	virtual void SystemPerEntityUpdate() = 0;
	virtual void SystemPostUpdate() = 0;
	void SetInitialized(bool initialized) { bInitialized = initialized; }
	void SetEnabled(bool enabled) { bEnabled = enabled; }

	void UpdateComponentAccessSets()
	{
		UpdateMatchingEntityTypes();
		FetchRequiredSpans();
	}

	void FetchRequiredSpans()
	{
		AccessedReadOnlyEntityTypes.clear();
		AccessedWriteReadEntityTypes.clear();
		AccessedReadOnlyComponentStorages.clear();
		AccessedWriteReadComponentStorages.clear();
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

		std::function<void()> fetcher = std::bind_front(&SystemBase::WriteReadComponentDataFetcher<ComponentType>, this, storage);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void WriteReadComponentDataFetcher(std::vector<std::span<ComponentType>>* storage)
	{
		std::vector<std::span<ComponentType>> newStorage;
		for (auto entityType : MatchingEntityTypes)
		{
			this->AccessedWriteReadEntityTypes.emplace_back(entityType);
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

		std::function<void()> fetcher = std::bind_front(&SystemBase::ReadOnlyComponentDataFetcher<ComponentType>, this, storage);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void ReadOnlyComponentDataFetcher(std::vector<std::span<ComponentType const>>* storage)
	{
		std::vector<std::span<ComponentType const>> newStorage;
		for (auto entityType : MatchingEntityTypes)
		{
			this->AccessedReadOnlyEntityTypes.emplace_back(entityType);
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
	void AddWriteReadAlwaysFetchEntityType(EntityType typeID, std::span<ComponentType>* storage)
	{
		auto& id = typeid(ComponentType);
		EntitySpanFetchInfo FetchedEntityInfo = WorldEntityManager->GetEntityFetchInfo(typeID);
		SystemEntityFetchInfo info = { .fetchInfo = FetchedEntityInfo , .componentType = id };
		AlwaysFetchedWriteReadComponentTypes.insert(id);

		std::function<void()> fetcher = std::bind_front(&SystemBase::WriteReadEntitiesFetcher<ComponentType>, this, typeID, storage);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void WriteReadEntitiesFetcher(EntityType typeID, std::span<ComponentType>* span)
	{
		this->AccessedWriteReadEntityTypes.emplace_back(typeID);
		std::span<ComponentType> fetchedSpan = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(typeID);
		*span = fetchedSpan;
	}

	template <typename ComponentType>
	void AddReadOnlyAlwaysFetchEntityType(EntityType typeID, std::span<ComponentType const>* storage)
	{
		auto& id = typeid(ComponentType);
		EntitySpanFetchInfo FetchedEntityInfo = WorldEntityManager->GetEntityFetchInfo(typeID);
		SystemEntityFetchInfo info = { .fetchInfo = FetchedEntityInfo , .componentType = id };
		AlwaysFetchedReadOnlyComponentTypes.insert(id);

		std::function<void()> fetcher = std::bind_front(&SystemBase::ReadOnlyEntitiesFetcher<ComponentType>, this, typeID, storage);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void ReadOnlyEntitiesFetcher(EntityType typeID, std::span<ComponentType const>* span)
	{
		this->AccessedReadOnlyEntityTypes.emplace_back(typeID);
		std::span<ComponentType const> fetchedSpan = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(typeID);
		*span = fetchedSpan;
	}

	template <typename ComponentType>
	void AddReadOnlySystemDataFetcher(std::span<ComponentType const>* span)
	{
		auto& id = typeid(ComponentType);
		ReadOnlySystemData.insert(id);

		std::function<void()> fetcher = std::bind_front(&SystemBase::ReadOnlySystemDataFetcher<ComponentType>, this, span);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void AddWriteReadSystemDataFetcher(std::span<ComponentType>* span)
	{
		auto& id = typeid(ComponentType);
		WriteReadSystemData.insert(id);

		std::function<void()> fetcher = std::bind_front(&SystemBase::WriteReadSystemDataFetcher<ComponentType>, this, span);
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

	template <typename ComponentType>
	void AddReadOnlyAlwaysFetchComponents(std::vector < std::span<ComponentType const>>* storage)
	{
		auto& id = typeid(ComponentType);
		ReadOnlyComponentStorages.insert(id);
		std::function<void()> fetcher = std::bind_front(&SystemBase::ReadOnlyComponentsFetcher<ComponentType>, this, storage);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void ReadOnlyComponentsFetcher(std::vector<std::span<ComponentType const>>* spans)
	{
		auto& id = typeid(ComponentType);
		this->AccessedReadOnlyComponentStorages.emplace_back(id);
		*spans = WorldComponentManager->GetReadOnlyComponentSpans<ComponentType>();
	}

	std::unordered_set<std::type_index> GetWriteReadAccessedEntityComponents()
	{
		std::unordered_set<std::type_index> accessed;
		accessed.insert(WriteReadSystemData.begin(), WriteReadSystemData.end());
		accessed.insert(WantedWriteReadComponentTypes.begin(), WantedWriteReadComponentTypes.end());
		accessed.insert(AlwaysFetchedWriteReadComponentTypes.begin(), AlwaysFetchedWriteReadComponentTypes.end());
		return accessed;
	}
	std::unordered_set<std::type_index> GetAllAccessedEntityComponents()
	{
		std::unordered_set<std::type_index> accessed;
		accessed.insert(ReadOnlySystemData.begin(), ReadOnlySystemData.end());
		accessed.insert(WriteReadSystemData.begin(), WriteReadSystemData.end());
		accessed.insert(WantedReadOnlyComponentTypes.begin(), WantedReadOnlyComponentTypes.end());
		accessed.insert(WantedWriteReadComponentTypes.begin(), WantedWriteReadComponentTypes.end());
		accessed.insert(AlwaysFetchedReadOnlyComponentTypes.begin(), AlwaysFetchedReadOnlyComponentTypes.end());
		accessed.insert(AlwaysFetchedWriteReadComponentTypes.begin(), AlwaysFetchedWriteReadComponentTypes.end());
		return accessed;
	}

	std::unordered_set<EntityType> GetReadOnlyAccessedEntityTypes()
	{
		std::unordered_set<EntityType> accessed;
		accessed.insert(AccessedReadOnlyEntityTypes.begin(), AccessedReadOnlyEntityTypes.end());
		return accessed;
	}
	std::unordered_set<EntityType> GetWriteReadAccessedEntityTypes()
	{
		std::unordered_set<EntityType> accessed;
		accessed.insert(AccessedWriteReadEntityTypes.begin(), AccessedWriteReadEntityTypes.end());
		accessed.insert(DestructiveEntityStorageAccess.begin(), DestructiveEntityStorageAccess.end());
		return accessed;
	}
	std::unordered_set<EntityType> GetAllAccessedEntityTypes()
	{
		std::unordered_set<EntityType> accessed;
		accessed.insert(AccessedReadOnlyEntityTypes.begin(), AccessedReadOnlyEntityTypes.end());
		accessed.insert(AccessedWriteReadEntityTypes.begin(), AccessedWriteReadEntityTypes.end());
		accessed.insert(DestructiveEntityStorageAccess.begin(), DestructiveEntityStorageAccess.end());
		return accessed;
	}

	std::unordered_set<std::type_index> GetWriteReadAccessedComponentStorages()
	{
		std::unordered_set<std::type_index> accessed;
		accessed.insert(AccessedReadOnlyComponentStorages.begin(), AccessedReadOnlyComponentStorages.end());
		return accessed;
	}

	std::unordered_set<std::type_index> GetAllAccessedComponentStorages()
	{
		std::unordered_set<std::type_index> accessed;
		accessed.insert(AccessedReadOnlyComponentStorages.begin(), AccessedReadOnlyComponentStorages.end());
		accessed.insert(AccessedWriteReadComponentStorages.begin(), AccessedWriteReadComponentStorages.end());
		return accessed;
	}

	// Variables:
protected:

	SystemType Type = SystemType::System;
	SystemSyncPoint RunSyncPoint = SystemSyncPoint::AfterEngineSystems;
	std::string RunBeforeSpecificSystem = "";
	std::string RunAfterSpecificSystem = "";

	std::vector<EntitySpanFetchInfo> FetchedEntitiesInfo;
	std::vector<EntityType> MatchingEntityTypes;

private:

	bool RunAlone = true;
	double CurrentDeltaTime = 0.0;

	ThreadedTaskRunner* ThreadRunner;
	std::thread::id MainThreadID;

	// For accessing entities that are required to be always fetched 
	std::unordered_set<std::type_index> AlwaysFetchedReadOnlyComponentTypes;
	std::unordered_set<std::type_index> AlwaysFetchedWriteReadComponentTypes;

	// For system data access.
	std::unordered_set<std::type_index> ReadOnlySystemData;
	std::unordered_set<std::type_index> WriteReadSystemData;

	// For dynamic entity fetching
	std::unordered_set<std::type_index> WantedReadOnlyComponentTypes;
	std::unordered_set<std::type_index> WantedWriteReadComponentTypes;
	std::unordered_set<std::type_index> ForbiddenComponentTypes;

	// For component storage access.
	std::unordered_set<std::type_index> ReadOnlyComponentStorages;
	std::unordered_set<std::type_index> WriteReadComponentStorages;

	// For system data access.
	std::unordered_set<EntityType> DestructiveEntityStorageAccess;

	// Logs all entity types that are accessed with fetch-functions
	std::vector<EntityType> AccessedReadOnlyEntityTypes;
	std::vector<EntityType> AccessedWriteReadEntityTypes;

	// Logs all component storages that are accessed with fetch-functions
	std::vector<std::type_index> AccessedReadOnlyComponentStorages;
	std::vector<std::type_index> AccessedWriteReadComponentStorages;


	std::vector<std::function<void()>> FetchFunctions;

	SystemID ID = -1;
	bool bInitialized = false;
	bool bEnabled = false;
	bool FetchSafetyChecks;

	EntityManager* WorldEntityManager;
	ComponentManager* WorldComponentManager;
};

export
template <typename T>
concept SystemBaseClass = std::derived_from<T, SystemBase>;
