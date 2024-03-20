module;

#include <fmt/core.h>

export module EncosyCore.SystemManager;

import EncosyCore.SharedBetweenManagers;
import EncosyCore.SystemBase;
import EncosyCore.System;
import EncosyCore.SystemThreaded;
import EncosyCore.Entity;
import EncosyCore.EntityManager;
import EncosyCore.ComponentManager;
import EncosyCore.ThreadedTaskRunner;

import <map>;
import <vector>;
import <list>;
import <memory>;
import <string>;
import <iostream>;
import <format>;
import <ranges>;
import <typeindex>;
import <typeinfo>;
import <unordered_set>;
import <algorithm>;
import <thread>;


export class SystemManager
{
public:

	friend class EncosyCore;

	SystemManager(SharedBetweenManagers* sharedBetweenManagers, ComponentManager* componentManager, EntityManager* entityManager)
	{
		WorldSharedBetweenManagers = sharedBetweenManagers;
		WorldComponentManager = componentManager;
		WorldEntityManager = entityManager;
		MainThreadID = std::this_thread::get_id();
	}
	~SystemManager() {}

	template <SystemBaseClass T, typename... Args>
	T* AddSystem(std::string systemName, Args...args)
	{
		//If system already exists, return existing
		if (SystemNames.find(systemName) != SystemNames.end())
		{
			fmt::println("ERROR: System with the name already exists : {}", systemName);
			return nullptr;
		}

		auto id = Systems.size();
		auto system = std::make_unique<T>(std::move(T(args...)));

		system->ID = id;
		system->WorldComponentManager = WorldComponentManager;
		system->WorldEntityManager = WorldEntityManager;
		system->ThreadRunner = &ThreadRunner;
		system->MainThreadID = MainThreadID;
		InitSystem(system.get(), systemName);

		ThreadingProtectionChecks(system.get(), systemName);

		system->SetInitialized(true);
		system->SetEnabled(true);

		auto returnSystem = system.get();

		Systems.push_back(std::move(system));
		std::type_index systemTypeIndex = typeid(T);
		SystemNames.insert(std::pair(systemName, id));
		SystemIDs.insert(std::pair(systemTypeIndex, id));

		ExecutionOrderRecalculationNeeded_ = true;
		return returnSystem;
	}

	template <SystemClass T>
	void InitSystem(T* system, std::string systemName)
	{
		fmt::println("Initializing system {}", systemName);
		system->SystemInit();
		system->Init();
	}

	template <SystemThreadedClass T>
	void InitSystem(T* system, std::string systemName)
	{
		fmt::println("Initializing threaded system {}", systemName);
		system->ThreadCount = GetThreadCount();
		system->RuntimeThreadInfo = std::vector<SystemThreadInfo>(system->ThreadCount, SystemThreadInfo());
		system->SystemInit();
		system->Init();
	}

	template <SystemClass T>
	void ThreadingProtectionChecks(T* system, std::string systemName)
	{
		CheckComponentAccesses(system, systemName);
	}

	template <SystemThreadedClass T>
	void ThreadingProtectionChecks(T* system, std::string systemName)
	{
		CheckComponentAccesses(system, systemName);
		std::unordered_set<std::type_index> wantedReadOnly = CompileWantedReadOnlyAccess(system);
		if (system->AllowDestructiveEditsInThreads)
		{
			auto readOnlyEntities = system->GetReadOnlyAccessedEntityTypes();
			auto nextDestructiveAccess = system->DestructiveEntityStorageAccess;

			bool destructiveCollision = false;
			for (const auto& access : nextDestructiveAccess)
			{
				if (std::ranges::find(readOnlyEntities, access) != readOnlyEntities.end())
				{
					destructiveCollision = true;
					break;
				}
			}
			if (destructiveCollision)
			{
				// ReadOnly currently reads directly from original storages with spans. 
				// AllowDestructiveEditsInThreads gives the thread ability to modify the original storages which can invalidate the spans used with readonly.
				// This could be avoided by allowing systems to only make destructive edits to entities designated to them and caching them until the end of UpdatePerEntity.
				// The changes could be then applied in a thread safe manner.
				fmt::println("ERROR when creating {}: When AllowDestructiveEditsInThreads is set to true, system cannot readonly fetch entity types it makes destructive edits to.", systemName);
				abort();
			}
		}
	}

	template <SystemBaseClass T>
	bool CheckComponentAccesses(T* system, std::string systemName)
	{
		WorldComponentManager->InitThreadingProtectionCheck();

		system->UpdateMatchingEntityTypes();
		system->FetchRequiredSpans();
		
		std::unordered_set<std::type_index> wantedReadOnly = CompileWantedReadOnlyAccess(system);
		std::unordered_set<std::type_index> wantedWriteRead = CompileWantedWriteReadAccess(system);

		const bool readOnlyIsEqual = (wantedReadOnly == WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed);
		const bool writeReadIsEqual = (wantedWriteRead == WorldComponentManager->ThreadingProtectionWriteReadComponentsAccessed);

		if (!readOnlyIsEqual)
		{
			fmt::println("ERROR when creating {}: WantedReadOnlyComponentTypes is different compared to what the system accesses", systemName);
			fmt::println("{} != {}", GetSetAsString(wantedReadOnly), GetSetAsString(WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed));

		}
		if (!writeReadIsEqual)
		{
			fmt::println("ERROR when creating {}: WantedWriteReadComponentTypes is different compared to what the system accesses", systemName);
			fmt::println("{} != {}", GetSetAsString(wantedWriteRead), GetSetAsString(WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed));
		}

		WorldComponentManager->FinishThreadingProtectionCheck();

		if ((readOnlyIsEqual && writeReadIsEqual) == false)
		{
			abort();
		}

		return true;
	}

	template <SystemBaseClass T>
	std::unordered_set<std::type_index> CompileWantedReadOnlyAccess(T* system)
	{
		std::unordered_set<std::type_index> wantedReadOnly;
		auto readOnlySet = system->WantedReadOnlyComponentTypes;
		wantedReadOnly.insert(readOnlySet.begin(), readOnlySet.end());
		readOnlySet = system->ReadOnlySystemData;
		wantedReadOnly.insert(readOnlySet.begin(), readOnlySet.end());
		readOnlySet = system->AlwaysFetchedReadOnlyComponentTypes;
		wantedReadOnly.insert(readOnlySet.begin(), readOnlySet.end());
		readOnlySet = system->ReadOnlyComponentStorages;
		wantedReadOnly.insert(readOnlySet.begin(), readOnlySet.end());
		return wantedReadOnly;
	}

	template <SystemBaseClass T>
	std::unordered_set<std::type_index> CompileWantedWriteReadAccess(T* system)
	{
		std::unordered_set<std::type_index> wantedWriteRead;
		auto writeReadSet = system->WantedWriteReadComponentTypes;
		wantedWriteRead.insert(writeReadSet.begin(), writeReadSet.end());
		writeReadSet = system->WriteReadSystemData;
		wantedWriteRead.insert(writeReadSet.begin(), writeReadSet.end());
		writeReadSet = system->AlwaysFetchedWriteReadComponentTypes;
		wantedWriteRead.insert(writeReadSet.begin(), writeReadSet.end());
		writeReadSet = system->WriteReadComponentStorages;
		wantedWriteRead.insert(writeReadSet.begin(), writeReadSet.end());
		return wantedWriteRead;
	}

	bool DisableSystem(SystemID systemID) const
	{
		if (systemID < Systems.size())
		{
			Systems.at(systemID)->SetEnabled(false);
			return true;
		}
		return false;
	}

	bool EnableSystem(SystemID systemID) const
	{
		if (systemID < Systems.size())
		{
			Systems.at(systemID)->SetEnabled(true);
			return true;
		}
		return false;
	}

	SystemBase* GetSystemById(const SystemID systemId) const
	{
		for (auto const& system : Systems)
		{
			if (system->ID == systemId)
			{
				return system.get();
			}
		}
		return nullptr;
	}

	std::string GetSystemNameById(const SystemID systemId) const
	{
		for (auto it = SystemNames.begin(); it != SystemNames.end(); ++it)
		{
			if (it->second == systemId)
			{
				return it->first;
			}
		}
		return "";
	}

	SystemBase* GetSystemByName(const std::string& systemName)
	{
		const auto it = SystemNames.find(systemName);
		if (it != SystemNames.end())
		{
			return GetSystemById(it->second);
		}
		return nullptr;
	}

	template <typename SystemClass>
	SystemClass* GetSystem()
	{
		const std::type_index systemTypeIndex = typeid(SystemClass);
		if (SystemIDs.find(systemTypeIndex) != SystemIDs.end())
		{
			return static_cast<SystemClass>(GetSystemById(SystemIDs.find(systemTypeIndex)->second));
		}
		return nullptr;
	}

	int GetThreadCount()
	{
		return ThreadRunner.GetThreadCount();
	}

	void QuerySystemOrderRecalculation()
	{
		ExecutionOrderRecalculationNeeded_ = true;
	}

protected:


	void UpdatePhysicsSystems(const double deltaTime)
	{
		// Will be converted to execute system batches in the future
		for (const auto& systemRunBatch : PhysicsSystemBatches)
		{
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemPreUpdate(deltaTime);
			}
			ThreadRunner.RunAllTasks();
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemUpdate();
			}
			ThreadRunner.RunAllTasks();
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemPerEntityUpdate();
			}
			ThreadRunner.RunAllTasks();
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemPostUpdate();
			}
			ThreadRunner.RunAllTasks();
		}
	}

	void UpdateSystems(const double deltaTime)
	{
		for (const auto& systemRunBatch : RegularSystemBatches)
		{
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemPreUpdate(deltaTime);
			}
			ThreadRunner.RunAllTasks();
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemUpdate();
			}
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemPerEntityUpdate();
			}
			ThreadRunner.RunAllTasks();
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemPostUpdate();
			}
			ThreadRunner.RunAllTasks();
		}
	}

	void UpdateRenderSystems(const double deltaTime)
	{
		// Will be converted to execute system batches in the future
		for (const auto& systemRunBatch : RenderSystemBatches)
		{
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemPreUpdate(deltaTime);
			}
			ThreadRunner.RunAllTasks();
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemUpdate();
			}
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemPerEntityUpdate();
			}
			ThreadRunner.RunAllTasks();
			for (const auto& systemId : systemRunBatch)
			{
				Systems[systemId]->SystemPostUpdate();
			}
			ThreadRunner.RunAllTasks();
		}
	}

	void ManagerUpdate()
	{
		if (!WorldSharedBetweenManagers->IsCurrentComposition(CurrentWorldCompositionNumber_)) 
		{
			ExecutionOrderRecalculationNeeded_ = true;
			CurrentWorldCompositionNumber_ = WorldSharedBetweenManagers->GetCurrentComposition();
		}

		if (ExecutionOrderRecalculationNeeded_)
		{
			fmt::println("Rebuilding system execute order batches");
			PhysicsSystemBatches = ReorderSystems(SystemType::PhysicsSystem);
			RegularSystemBatches = ReorderSystems(SystemType::System);
			RenderSystemBatches = ReorderSystems(SystemType::RenderSystem);
			fmt::println("PhysicsSystemBatches:");
			BatchSystemNamesPrint(PhysicsSystemBatches);
			fmt::println("RegularSystemBatches:");
			BatchSystemNamesPrint(RegularSystemBatches);
			fmt::println("RenderSystemBatches:");
			BatchSystemNamesPrint(RenderSystemBatches);
			ExecutionOrderRecalculationNeeded_ = false;
		}
	}

	std::vector<std::vector<SystemID>> ReorderSystems(SystemType systemType)
	{
		std::vector<std::vector<SystemID>> systemBatches;

		for (int syncPoint = static_cast<int>(SystemSyncPoint::First); syncPoint < static_cast<int>(SystemSyncPoint::Last); syncPoint++)
		{
			std::list<SystemDependencies> allDependencies;
			
			// Add all systems with correct type for handling
			for (auto& system : Systems)
			{
				auto systemPtr = system.get();
				if (systemPtr->GetType() != systemType) { continue; }
				if (static_cast<int>(systemPtr->RunSyncPoint) != syncPoint) { continue; }
				allDependencies.push_back({ systemPtr->GetID(), std::vector<SystemID> ()});
			}

			// Sum up all dependencies by after which system each system wants to run
			for (auto& system : Systems)
			{
				auto systemPtr = system.get();
				if (systemPtr->GetType() != systemType) { continue; }
				if (static_cast<int>(systemPtr->RunSyncPoint) != syncPoint) { continue; }

				if (systemPtr->RunAfterSpecificSystem != "")
				{
					auto afterDependency = SystemNames.find(systemPtr->RunAfterSpecificSystem);
					if(afterDependency == SystemNames.end())
					{
						fmt::println("ERROR: System {} has invalid RunAfterSpecificSystem: {}", GetSystemNameById(systemPtr->GetID()), systemPtr->RunAfterSpecificSystem);
					}
					SystemID afterSystem = afterDependency->second;
					auto& dependenciesList = *std::ranges::find(allDependencies, systemPtr->GetID(), &SystemDependencies::system);
					dependenciesList.runAfter.push_back(afterSystem);
				}
				if (systemPtr->RunBeforeSpecificSystem != "")
				{
					auto beforeDependency = SystemNames.find(systemPtr->RunBeforeSpecificSystem);
					if (beforeDependency == SystemNames.end())
					{
						fmt::println("ERROR: System {} has invalid RunBeforeSpecificSystem: {}", GetSystemNameById(systemPtr->GetID()), systemPtr->RunBeforeSpecificSystem);
					}
					SystemID beforeSystem = beforeDependency->second;
					auto& dependenciesList = *std::ranges::find(allDependencies, beforeSystem, &SystemDependencies::system);
					for (auto dependency : dependenciesList.runAfter)
					{
						auto& subDependency = *std::ranges::find(allDependencies, dependency, &SystemDependencies::system);
						subDependency.runAfter.push_back(systemPtr->GetID());
					}
					dependenciesList.runAfter.push_back(systemPtr->GetID());
				}
			}

			// Check for cyclic dependencies
			for (const auto& first : allDependencies)
			{
				for (const auto& second : allDependencies)
				{
					if (first.system == second.system) { continue; }
					auto asDependecyToSecond = std::ranges::find(second.runAfter, first.system);
					auto asDependecyToFirst = std::ranges::find(first.runAfter, second.system);
					if (asDependecyToSecond != second.runAfter.end() && asDependecyToFirst != first.runAfter.end())
					{
						fmt::println("Circular system dependecy found: {} and {} both want to execute after each other!", GetSystemNameById(first.system), GetSystemNameById(second.system));
						abort();
					}
				}
			}

			// Sort based on who has the most dependencies
			allDependencies.sort([](const auto& A, const auto& B)
				{
					return A.runAfter.size() > B.runAfter.size();
				});

			// Sort until all systems have an order where they are executed after the systems that depend on them
			bool sorting = true;
			while (sorting)
			{
				sorting = false;
				for (auto firstit = allDependencies.begin(); firstit != allDependencies.end(); firstit++)
				{
					auto secondIt = allDependencies.begin();
					bool nextToRequired = true;
					while (secondIt != allDependencies.end())
					{
						if(firstit->system == secondIt->system) { nextToRequired = true; }
						auto sameDep = std::ranges::find_if(secondIt->runAfter, [&](const auto v) {
							return(std::ranges::find(firstit->runAfter, v) != firstit->runAfter.end());
						});
						if (sameDep != secondIt->runAfter.end())
						{
							secondIt++;
							continue;
						}
						auto runAfter = std::ranges::find(firstit->runAfter, secondIt->system);
						if (runAfter != firstit->runAfter.end())
						{
							if (!nextToRequired)
							{
								allDependencies.splice(secondIt, allDependencies, firstit);
								sorting = true;
							}
							secondIt++;
							continue;
						}
						nextToRequired = false;
						secondIt++;
					}
				}
			}


			std::vector<SystemID> systemBatch;
			std::unordered_set<std::type_index> batchComponentAccess;
			std::unordered_set<EntityType> batchEntityAccess;
			std::unordered_set<std::type_index> batchStorageAccess;
			for (auto itCurrent = allDependencies.rbegin(); itCurrent != allDependencies.rend(); itCurrent++)
			{
				systemBatch.push_back(itCurrent->system);
				batchComponentAccess.merge(Systems[itCurrent->system]->GetAllAccessedEntityComponents());
				batchEntityAccess.merge(Systems[itCurrent->system]->GetAllAccessedEntityTypes());
				auto accessedComponentStorages = Systems[itCurrent->system]->GetAllAccessedComponentStorages();
				for (const auto& access : accessedComponentStorages)
				{
					auto entityTypes = WorldEntityManager->GetEntityTypesWithComponent(access);
					for (const auto entityType : entityTypes)
					{
						batchEntityAccess.insert(entityType);
					}
				}
				batchStorageAccess.merge(accessedComponentStorages);
				auto itNext = itCurrent;
				itNext++;

				// If we are at the end of system list, end this batch.
				if (itNext == allDependencies.rend())
				{
					systemBatches.push_back(systemBatch);
					break;
				}
				// If current system wants to be executed alone, end this batch.
				if (Systems[itCurrent->system]->GetRunAlone())
				{
					systemBatches.push_back(systemBatch);
					systemBatch.clear();
					batchComponentAccess.clear();
					batchEntityAccess.clear();
					batchStorageAccess.clear();
					continue;
				}
				// If next system wants to run alone end the current batch.
				if (Systems[itNext->system]->GetRunAlone())
				{
					systemBatches.push_back(systemBatch);
					systemBatch.clear();
					batchComponentAccess.clear();
					batchEntityAccess.clear();
					batchStorageAccess.clear();
					continue;
				}
				// If next system has current system as dependency, end this batch.
				if (std::ranges::find(itNext->runAfter, itCurrent->system) != itNext->runAfter.end())
				{
					systemBatches.push_back(systemBatch);
					systemBatch.clear();
					batchComponentAccess.clear();
					batchEntityAccess.clear();
					batchStorageAccess.clear();
					continue;
				}

				// If system modifies same component storages as the current batch, end this batch.

				std::unordered_set<std::type_index> nextStorageAccess = Systems[itNext->system]->GetWriteReadAccessedComponentStorages();
				bool collisionFound = false;
				for (const auto& access : nextStorageAccess)
				{
					if (std::ranges::find(batchStorageAccess, access) != batchStorageAccess.end())
					{
						collisionFound = true;
						break;
					}
				}
				if (collisionFound) // Next system writes to a component that this batch already processes.
				{
					systemBatches.push_back(systemBatch);
					systemBatch.clear();
					batchComponentAccess.clear();
					batchEntityAccess.clear();
					batchStorageAccess.clear();
					continue;
				}

				// If nex system component storage is used in current batch entities, end this batch
				collisionFound = false;
				for (const auto& access : nextStorageAccess)
				{
					auto entityTypes = WorldEntityManager->GetEntityTypesWithComponent(access);
					for(const auto entityType : entityTypes)
					{
						if(batchEntityAccess.contains(entityType))
						{
							collisionFound = true;
							break;
						}
						if (collisionFound) { break; }
					}
				}
				if (collisionFound) // Next system writes to a component that this batch already processes.
				{
					systemBatches.push_back(systemBatch);
					systemBatch.clear();
					batchComponentAccess.clear();
					batchEntityAccess.clear();
					batchStorageAccess.clear();
					continue;
				}

				// If next system modifies same components as the current batch, end this batch.
				std::unordered_set<std::type_index> nextAccess = Systems[itNext->system]->GetWriteReadAccessedEntityComponents();
				std::unordered_set<EntityType> nextEntityAccess = Systems[itNext->system]->GetWriteReadAccessedEntityTypes();

				bool possibleCollision = false;
				for (const auto& access : nextAccess)
				{
					if (std::ranges::find(batchComponentAccess, access) != batchComponentAccess.end())
					{
						possibleCollision = true;
						break;
					}
				}
				if (possibleCollision) // Check if the colliding component is from entity types the batch already processes
				{

					for (const auto& access : nextEntityAccess)
					{
						if (std::ranges::find(batchEntityAccess, access) != batchEntityAccess.end())
						{
							collisionFound = true;
							break;
						}
					}
					if (collisionFound) // Next system writes to a component that this batch already processes.
					{
						systemBatches.push_back(systemBatch);
						systemBatch.clear();
						batchComponentAccess.clear();
						batchEntityAccess.clear();
						batchStorageAccess.clear();
						continue;
					}
				}
			}
		}
		return systemBatches;
	}

	void DestroySystems() const
	{
		for (size_t i = 0; i < Systems.size(); i++)
		{
			Systems[i]->Destroy();
		}
	}

private:

	struct SystemDependencies
	{
		SystemID system;
		std::vector<SystemID> runAfter;
	};


	static std::string GetSetAsString(const std::unordered_set<std::type_index>& set)
	{
		std::string s;
		for (const auto& elem : set)
		{
			s += elem.name();
			s += ", ";
		}
		return s;
	}

	void BatchSystemNamesPrint(std::vector<std::vector<SystemID>> batch)
	{
		for (const auto& vec : batch)
		{
			fmt::print("[");
			for(const auto& id : vec)
			{
				fmt::print("{},", this->GetSystemNameById(id));

			}
			fmt::print("],");
		}
		fmt::println("");
	}

	bool ExecutionOrderRecalculationNeeded_ = false;

	SharedBetweenManagers* WorldSharedBetweenManagers;
	EntityManager* WorldEntityManager;
	ComponentManager* WorldComponentManager;
	std::vector<std::unique_ptr<SystemBase>> Systems;
	std::map<std::string, SystemID> SystemNames;
	std::map<std::type_index, SystemID> SystemIDs;

	std::vector<std::vector<SystemID>> PhysicsSystemBatches;
	std::vector<std::vector<SystemID>> RegularSystemBatches;
	std::vector<std::vector<SystemID>> RenderSystemBatches;

	// System threading
	ThreadedTaskRunner ThreadRunner;
	std::thread::id MainThreadID;

	// Used mainly to signal other managers to rebuild critical structures
	size_t CurrentWorldCompositionNumber_ = 0;
};
