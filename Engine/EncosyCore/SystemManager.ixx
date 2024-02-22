module;

#include <fmt/core.h>

export module ECS.SystemManager;

import ECS.System;
import ECS.Entity;
import ECS.EntityManager;
import ECS.ComponentManager;
import EncosyCore.ThreadedTaskRunner;

import <map>;
import <vector>;
import <memory>;
import <string>;
import <iostream>;
import <format>;
import <ranges>;
import <typeindex>;
import <typeinfo>;
import <unordered_set>;

export class SystemManager
{
public:

	friend class EncosyCore;

	SystemManager(ComponentManager* componentManager, EntityManager* entityManager) : WorldComponentManager(componentManager), WorldEntityManager(entityManager) {}
	~SystemManager() {}

	template <SystemBaseClass T, typename... Args>
	T* AddSystem(std::string systemName, Args...args)
	{
		//If system already exists, return existing
		auto it = SystemNames.find(systemName);
		if (it != SystemNames.end())
		{
			fmt::println("ERROR: System with the name already exists : {}", systemName);
			return nullptr;
		}

		auto id = Systems.size();
		auto system = std::make_unique<T>(T(args...));

		system->ID = id;
		system->WorldComponentManager = WorldComponentManager;
		system->WorldEntityManager = WorldEntityManager;
		InitSystem(system.get(), systemName);

		ThreadingProtectionChecks(system.get(), systemName);

		system->SetInitialized(true);
		system->SetEnabled(true);

		auto returnSystem = system.get();

		Systems.push_back(std::move(system));
		std::type_index systemTypeIndex = typeid(T);
		SystemNames.insert(std::pair(systemName, id));
		SystemIDs.insert(std::pair(systemTypeIndex, id));

		return returnSystem;
	}

	template <SystemClass T>
	void InitSystem(T* system, std::string systemName)
	{
		fmt::println("Initializing system ", systemName);
		system->Init();
	}

	template <ThreadedSystemClass T>
	void InitSystem(T* system, std::string systemName)
	{
		fmt::println("Initializing threaded system ", systemName);
		system->ThreadCount = GetThreadCount();
		system->RuntimeThreadInfo = std::vector<ThreadInfo>(system->ThreadCount, ThreadInfo());
		system->ThreadRunner = &ThreadRunner;
		system->Init();
	}

	template <SystemBaseClass T>
	bool ThreadingProtectionChecks(T* system, std::string systemName)
	{
		WorldComponentManager->InitThreadingProtectionCheck();

		system->UpdateMatchingEntityTypes();
		system->FetchRequiredSpans();

		auto wantedReadOnly = system->WantedReadOnlyComponentTypes;
		wantedReadOnly.merge(system->ReadOnlySystemData);
		wantedReadOnly.merge(system->AlwaysFetchedReadOnlyComponentTypes);

		auto wantedWriteRead = system->WantedWriteReadComponentTypes;
		wantedWriteRead.merge(system->WriteReadSystemData);
		wantedReadOnly.merge(system->AlwaysFetchedWriteReadComponentTypes);

		bool readOnlyIsEqual = (wantedReadOnly == WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed);
		bool WriteReadIsEqual = (wantedWriteRead == WorldComponentManager->ThreadingProtectionWriteReadComponentsAccessed);

		if (!readOnlyIsEqual)
		{
			fmt::println("ERROR when creating {}: WantedReadOnlyComponentTypes is different compared to what the system accesses", systemName);
			fmt::println("{} != {}", GetSetAsString(wantedReadOnly), GetSetAsString(WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed));

		}
		if (!WriteReadIsEqual)
		{
			fmt::println("ERROR when creating {}: WantedWriteReadComponentTypes is different compared to what the system accesses", systemName);
			fmt::println("{} != {}", GetSetAsString(wantedWriteRead), GetSetAsString(WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed));
		}

		WorldComponentManager->FinishThreadingProtectionCheck();

		if ((readOnlyIsEqual && WriteReadIsEqual) == false)
		{
			abort();
		}

		return true;
	}

	bool DisableSystem(SystemID systemID)
	{
		if (systemID < Systems.size())
		{
			Systems.at(systemID)->SetEnabled(false);
			return true;
		}
		return false;
	}

	bool EnableSystem(SystemID systemID)
	{
		if (systemID < Systems.size())
		{
			Systems.at(systemID)->SetEnabled(true);
			return true;
		}
		return false;
	}

	SystemBase* GetSystemById(SystemID systemId)
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

	SystemBase* GetSystemByName(std::string systemName)
	{
		auto it = SystemNames.find(systemName);
		if (it != SystemNames.end())
		{
			return GetSystemById(it->second);
		}
		return nullptr;
	}

	template <typename SystemClass>
	SystemClass* GetSystem()
	{
		std::type_index systemTypeIndex = typeid(SystemClass);
		auto it = SystemIDs.find(systemTypeIndex);
		if (it != SystemIDs.end())
		{
			return static_cast<SystemClass>(GetSystemById(it->second));
		}
		return nullptr;
	}

	int GetThreadCount()
	{
		return ThreadRunner.GetThreadCount();
	}

protected:
	void UpdateSystems(float deltaTime)
	{
		// PreUpdate
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::System)
			{
				Systems.at(i)->PreUpdate(deltaTime);
			}
			else if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::ThreadedSystem)
			{
				Systems.at(i)->PreUpdate(deltaTime);
			}
		}

		// Update
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::System)
			{
				Systems.at(i)->SystemUpdate(deltaTime);
			}
			else if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::ThreadedSystem)
			{
				Systems.at(i)->SystemUpdate(deltaTime);
			}
		}

		// PostUpdate
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::System)
			{
				Systems.at(i)->PostUpdate(deltaTime);
			}
			else if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::ThreadedSystem)
			{
				Systems.at(i)->PostUpdate(deltaTime);
			}
		}
	}

	void UpdateRenderSystems(float deltaTime)
	{
		// PreUpdate
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::RenderSystem)
			{
				Systems.at(i)->PreUpdate(deltaTime);
			}
		}
		// Update
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::RenderSystem)
			{
				Systems.at(i)->SystemUpdate(deltaTime);
			}
		}

		// PostUpdate
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::RenderSystem)
			{
				Systems.at(i)->PostUpdate(deltaTime);
			}
		}
	}

	void UpdatePhysicsSystems(float deltaTime)
	{
		// PreUpdate
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::PhysicsSystem)
			{
				Systems.at(i)->PreUpdate(deltaTime);
			}
		}
		// Update
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::PhysicsSystem)
			{
				Systems.at(i)->SystemUpdate(deltaTime);
			}
		}

		// PostUpdate
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::PhysicsSystem)
			{
				Systems.at(i)->PostUpdate(deltaTime);
			}
		}
	}

	void DestroySystems()
	{
		for (size_t i = 0; i < Systems.size(); i++)
		{
			Systems[i]->Destroy();
		}
	}

private:

	bool CheckSystemType(System* system, SystemType approved...)
	{

	}

	std::string GetSetAsString(const std::unordered_set<std::type_index>& set)
	{
		std::string s;
		for (const auto& elem : set)
		{
			s += elem.name();
			s += ", ";
		}
		return s;
	}

	EntityManager* WorldEntityManager;
	ComponentManager* WorldComponentManager;
	std::vector<std::unique_ptr<SystemBase>> Systems;
	std::map<std::string, SystemID> SystemNames;
	std::map<std::type_index, SystemID> SystemIDs;

	// System threading
	ThreadedTaskRunner ThreadRunner;
};
