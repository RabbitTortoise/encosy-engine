module;
#include <fmt/core.h>
#include <array>

export module EE_Encosy_SystemBase;

import EE_Encosy_Entity;
import EE_Encosy_ComponentManager;
import EE_Encosy_ComponentTypeStorage;
import EE_Encosy_EntityManager;
import EE_Encosy_EntityTypeStorage;
import EE_Encosy_ThreadedTaskRunner;


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

public:
	SystemBase() {}
	virtual ~SystemBase() {}
	virtual void Init() = 0;
	virtual void Destroy() = 0;

	virtual void SystemPreUpdate(double deltaTime) = 0;
	virtual void SystemUpdate() = 0;
	virtual void SystemPerEntityUpdate() = 0;
	virtual void SystemPostUpdate() = 0;

	void InitSystemBase(SystemID id, ComponentManager* componentManager, EntityManager* entityManager, std::thread::id mainThreadID)
	{
		ID = id;
		WorldComponentManager = componentManager;
		WorldEntityManager = entityManager;
		MainThreadID = mainThreadID;
	}

	std::unordered_set<std::type_index> GetWriteReadAccessedEntityComponents()
	{
		std::unordered_set<std::type_index> accessed;
		accessed.insert(WriteReadSystemDataList.begin(), WriteReadSystemDataList.end());
		accessed.insert(FetchedWriteReadComponentTypesList.begin(), FetchedWriteReadComponentTypesList.end());
		accessed.insert(AlwaysFetchedWriteReadComponentTypesList.begin(), AlwaysFetchedWriteReadComponentTypesList.end());
		return accessed;
	}
	std::unordered_set<std::type_index> GetAllAccessedEntityComponents()
	{
		std::unordered_set<std::type_index> accessed;
		accessed.insert(ReadOnlySystemDataList.begin(), ReadOnlySystemDataList.end());
		accessed.insert(WriteReadSystemDataList.begin(), WriteReadSystemDataList.end());
		accessed.insert(FetchedReadOnlyComponentTypesList.begin(), FetchedReadOnlyComponentTypesList.end());
		accessed.insert(FetchedWriteReadComponentTypesList.begin(), FetchedWriteReadComponentTypesList.end());
		accessed.insert(AlwaysFetchedReadOnlyComponentTypesList.begin(), AlwaysFetchedReadOnlyComponentTypesList.end());
		accessed.insert(AlwaysFetchedWriteReadComponentTypesList.begin(), AlwaysFetchedWriteReadComponentTypesList.end());
		return accessed;
	}

	std::unordered_set<EntityType> GetReadOnlyAccessedEntityTypes()
	{
		std::unordered_set<EntityType> accessed;
		accessed.insert(AccessedReadOnlyEntityTypesList.begin(), AccessedReadOnlyEntityTypesList.end());
		return accessed;
	}
	std::unordered_set<EntityType> GetWriteReadAccessedEntityTypes()
	{
		std::unordered_set<EntityType> accessed;
		accessed.insert(AccessedWriteReadEntityTypesList.begin(), AccessedWriteReadEntityTypesList.end());
		accessed.insert(DestructiveEntityStorageAccessList.begin(), DestructiveEntityStorageAccessList.end());
		return accessed;
	}
	std::unordered_set<EntityType> GetAllAccessedEntityTypes()
	{
		std::unordered_set<EntityType> accessed;
		accessed.insert(AccessedReadOnlyEntityTypesList.begin(), AccessedReadOnlyEntityTypesList.end());
		accessed.insert(AccessedWriteReadEntityTypesList.begin(), AccessedWriteReadEntityTypesList.end());
		accessed.insert(DestructiveEntityStorageAccessList.begin(), DestructiveEntityStorageAccessList.end());
		return accessed;
	}

	std::unordered_set<std::type_index> GetWriteReadAccessedComponentStorages()
	{
		std::unordered_set<std::type_index> accessed;
		accessed.insert(AccessedReadOnlyComponentStoragesList.begin(), AccessedReadOnlyComponentStoragesList.end());
		return accessed;
	}

	std::unordered_set<std::type_index> GetAllAccessedComponentStorages()
	{
		std::unordered_set<std::type_index> accessed;
		accessed.insert(AccessedReadOnlyComponentStoragesList.begin(), AccessedReadOnlyComponentStoragesList.end());
		accessed.insert(AccessedWriteReadComponentStoragesList.begin(), AccessedWriteReadComponentStoragesList.end());
		return accessed;
	}

	void UpdateMatchingEntityTypes()
	{
		MatchingEntityTypes.clear();
		WorldEntityManager->GetEntityTypesWithComponentConditions(MatchingEntityTypes, FetchedReadOnlyComponentTypesList, FetchedWriteReadComponentTypesList, RequiredComponentTypesList, ForbiddenComponentTypesList);
	}

	void FetchRequiredSpans()
	{
		AccessedReadOnlyEntityTypesList.clear();
		AccessedWriteReadEntityTypesList.clear();
		AccessedReadOnlyComponentStoragesList.clear();
		AccessedWriteReadComponentStoragesList.clear();
		for (auto& fetcher : FetchFunctions)
		{
			std::invoke(fetcher);
		}
	}

protected:

	template <typename ComponentType>
	void AddForbiddenComponentQuery()
	{
		auto& id = typeid(ComponentType);
		ForbiddenComponentTypesList.insert(id);
	}

	template <typename ComponentType>
	void AddRequiredComponentQuery()
	{
		auto& id = typeid(ComponentType);
		RequiredComponentTypesList.insert(id);
	}

	void EnableDestructiveAccessToEntityStorage(const EntityType entityType)
	{
		DestructiveEntityStorageAccessList.insert(entityType);
	}

	template <typename ComponentType>
	void AddWriteReadComponentFetcher(std::vector<std::span<ComponentType>>* storage)
	{
		auto& id = typeid(ComponentType);
		FetchedWriteReadComponentTypesList.insert(id);

		const std::function<void()> fetcher = std::bind_front(&SystemBase::WriteReadComponentDataFetcher<ComponentType>, this, storage);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void AddReadOnlyComponentFetcher(std::vector<std::span<ComponentType const>>* storage)
	{
		auto& id = typeid(ComponentType);
		FetchedReadOnlyComponentTypesList.insert(id);

		const std::function<void()> fetcher = std::bind_front(&SystemBase::ReadOnlyComponentDataFetcher<ComponentType>, this, storage);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void AddWriteReadAlwaysFetchEntityType(EntityType typeID, std::span<ComponentType>* storage)
	{
		auto& id = typeid(ComponentType);
		const EntitySpanFetchInfo FetchedEntityInfo = WorldEntityManager->GetEntityFetchInfo(typeID);
		SystemEntityFetchInfo info = { .fetchInfo = FetchedEntityInfo , .componentType = id };
		AlwaysFetchedWriteReadComponentTypesList.insert(id);

		const std::function<void()> fetcher = std::bind_front(&SystemBase::WriteReadEntitiesFetcher<ComponentType>, this, typeID, storage);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void AddReadOnlyAlwaysFetchEntityType(EntityType typeID, std::span<ComponentType const>* storage)
	{
		auto& id = typeid(ComponentType);
		const EntitySpanFetchInfo fetchedEntityInfo = WorldEntityManager->GetEntityFetchInfo(typeID);
		SystemEntityFetchInfo info = { .fetchInfo = fetchedEntityInfo , .componentType = id };
		AlwaysFetchedReadOnlyComponentTypesList.insert(id);

		const std::function<void()> fetcher = std::bind_front(&SystemBase::ReadOnlyEntitiesFetcher<ComponentType>, this, typeID, storage);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void AddReadOnlySystemDataFetcher(std::span<ComponentType const>* span)
	{
		auto& id = typeid(ComponentType);
		ReadOnlySystemDataList.insert(id);

		const std::function<void()> fetcher = std::bind_front(&SystemBase::ReadOnlySystemDataFetcher<ComponentType>, this, span);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void AddWriteReadSystemDataFetcher(std::span<ComponentType>* span)
	{
		auto& id = typeid(ComponentType);
		WriteReadSystemDataList.insert(id);

		const std::function<void()> fetcher = std::bind_front(&SystemBase::WriteReadSystemDataFetcher<ComponentType>, this, span);
		FetchFunctions.push_back(fetcher);
	}

	template <typename ComponentType>
	void AddReadOnlyAlwaysFetchComponents(std::vector < std::span<ComponentType const>>* storage)
	{
		auto& id = typeid(ComponentType);
		ReadOnlyComponentStoragesList.insert(id);
		const std::function<void()> fetcher = std::bind_front(&SystemBase::ReadOnlyComponentsFetcher<ComponentType>, this, storage);
		FetchFunctions.push_back(fetcher);
	}

private:

	void UpdateComponentAccessSets()
	{
		UpdateMatchingEntityTypes();
		FetchRequiredSpans();
	}

	template <typename ComponentType>
	void WriteReadComponentDataFetcher(std::vector<std::span<ComponentType>>* storage)
	{
		std::vector<std::span<ComponentType>> newStorage;
		for (auto entityType : MatchingEntityTypes)
		{
			this->AccessedWriteReadEntityTypesList.emplace_back(entityType);
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
	void ReadOnlyComponentDataFetcher(std::vector<std::span<ComponentType const>>* storage)
	{
		std::vector<std::span<ComponentType const>> newStorage;
		for (auto entityType : MatchingEntityTypes)
		{
			this->AccessedReadOnlyEntityTypesList.emplace_back(entityType);
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
	void WriteReadEntitiesFetcher(EntityType typeID, std::span<ComponentType>* span)
	{
		this->AccessedWriteReadEntityTypesList.emplace_back(typeID);
		std::span<ComponentType> fetchedSpan = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(typeID);
		*span = fetchedSpan;
	}

	template <typename ComponentType>
	void ReadOnlyEntitiesFetcher(EntityType typeID, std::span<ComponentType const>* span)
	{
		this->AccessedReadOnlyEntityTypesList.emplace_back(typeID);
		std::span<ComponentType const> fetchedSpan = WorldEntityManager->GetEntityReadOnlyComponentSpan<ComponentType>(typeID);
		*span = fetchedSpan;
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
	void ReadOnlyComponentsFetcher(std::vector<std::span<ComponentType const>>* spans)
	{
		auto& id = typeid(ComponentType);
		this->AccessedReadOnlyComponentStoragesList.emplace_back(id);
		*spans = WorldComponentManager->GetReadOnlyComponentSpans<ComponentType>();
	}

	// Variables:

public:
	SystemSyncPoint GetRunSyncPoint() const { return RunSyncPoint; }
	SystemType GetType() const { return Type; }
	SystemID GetID() const { return ID; }
	bool GetInitialized() const { return bInitialized; }
	bool GetEnabled() const { return bEnabled; }
	bool GetRunAlone() const { return RunAlone; }
	std::string GetRunBeforeSpecificSystem() const { return RunBeforeSpecificSystem; }
	std::string GetRunWithSpecificSystem() const { return RunWithSpecificSystem; }
	std::string GetRunAfterSpecificSystem() const { return RunAfterSpecificSystem; }

	std::unordered_set<std::type_index> GetAlwaysFetchedReadOnlyComponentTypesList() const { return AlwaysFetchedReadOnlyComponentTypesList; }
	std::unordered_set<std::type_index> GetAlwaysFetchedWriteReadComponentTypesList() const { return AlwaysFetchedWriteReadComponentTypesList; }
	std::unordered_set<std::type_index> GetReadOnlySystemDataList() const { return ReadOnlySystemDataList; }
	std::unordered_set<std::type_index> GetWriteReadSystemDataList() const { return WriteReadSystemDataList; }
	std::unordered_set<std::type_index> GetReadOnlyComponentStoragesList() const { return ReadOnlyComponentStoragesList; }
	std::unordered_set<std::type_index> GetWriteReadComponentStoragesList() const { return WriteReadComponentStoragesList; }
	std::unordered_set<std::type_index> GetFetchedReadOnlyComponentTypesList() const { return FetchedReadOnlyComponentTypesList; }
	std::unordered_set<std::type_index> GetFetchedWriteReadComponentTypesList() const { return FetchedWriteReadComponentTypesList; }
	std::unordered_set<std::type_index> GetForbiddenComponentTypesList() const { return ForbiddenComponentTypesList; }
	std::unordered_set<std::type_index> GetRequiredComponentTypesList() const { return RequiredComponentTypesList; }
	std::unordered_set<EntityType> GetDestructiveEntityStorageAccessList() const { return DestructiveEntityStorageAccessList; }
	std::vector<EntityType> GetAccessedReadOnlyEntityTypesList() const { return AccessedReadOnlyEntityTypesList; }
	std::vector<EntityType> GetAccessedWriteReadEntityTypesList() const { return AccessedWriteReadEntityTypesList; }
	std::vector<std::type_index> GetAccessedReadOnlyComponentStoragesList() const { return AccessedReadOnlyComponentStoragesList; }
	std::vector<std::type_index> GetAccessedWriteReadComponentStoragesList() const { return AccessedWriteReadComponentStoragesList; }



	void SetID(SystemID newId) { ID = newId; }
	void SetInitialized(bool initialized) { bInitialized = initialized; }
	void SetEnabled(bool enabled) { bEnabled = enabled; }


protected:

	SystemType Type = SystemType::System;
	SystemSyncPoint RunSyncPoint = SystemSyncPoint::AfterEngineSystems;
	std::string RunBeforeSpecificSystem = "";
	std::string RunWithSpecificSystem = "";
	std::string RunAfterSpecificSystem = "";

	std::vector<EntitySpanFetchInfo> FetchedEntitiesInfo;
	std::vector<EntityType> MatchingEntityTypes;

	EntityManager* WorldEntityManager = nullptr;
	ComponentManager* WorldComponentManager = nullptr;
	double CurrentDeltaTime = 0.0;
	bool RunAlone = true;

	std::thread::id MainThreadID;

	// For accessing entities that are required to be fetched regardless of query rules
	std::unordered_set<std::type_index> AlwaysFetchedReadOnlyComponentTypesList;
	std::unordered_set<std::type_index> AlwaysFetchedWriteReadComponentTypesList;

	// For accessing system data storages.
	std::unordered_set<std::type_index> ReadOnlySystemDataList;
	std::unordered_set<std::type_index> WriteReadSystemDataList;

	// For accessing entire component storages
	std::unordered_set<std::type_index> ReadOnlyComponentStoragesList;
	std::unordered_set<std::type_index> WriteReadComponentStoragesList;

	// For dynamic entity querying
	std::unordered_set<std::type_index> FetchedReadOnlyComponentTypesList;
	std::unordered_set<std::type_index> FetchedWriteReadComponentTypesList;
	std::unordered_set<std::type_index> ForbiddenComponentTypesList;
	std::unordered_set<std::type_index> RequiredComponentTypesList;

	// For managing rights to make destructive modifications directly to entity storages.
	std::unordered_set<EntityType> DestructiveEntityStorageAccessList;


	// TODO Check if these can be unordered_set instead of vector
	// Logs all entity types that are accessed with fetch-functions
	std::vector<EntityType> AccessedReadOnlyEntityTypesList;
	std::vector<EntityType> AccessedWriteReadEntityTypesList;

	// Logs all component storages that are accessed with fetch-functions
	std::vector<std::type_index> AccessedReadOnlyComponentStoragesList;
	std::vector<std::type_index> AccessedWriteReadComponentStoragesList;



private:

	std::vector<std::function<void()>> FetchFunctions;

	SystemID ID = -1;
	bool bInitialized = false;
	bool bEnabled = false;
	bool FetchSafetyChecks = true;

};

export
template <typename T>
concept SystemBaseClass = std::derived_from<T, SystemBase>;
