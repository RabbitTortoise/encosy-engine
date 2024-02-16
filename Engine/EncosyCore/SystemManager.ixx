module;

#include <fmt/core.h>

export module ECS.SystemManager;

import ECS.System;
import ECS.Entity;
import ECS.EntityManager;
import ECS.ComponentManager;

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

	template <class SystemClass, typename... Args>
	SystemClass* AddSystem(std::string systemName, bool allowThreading = false, Args...args)
	{
		//If system already exists, return existing
		auto it = SystemNames.find(systemName);
		if (it != SystemNames.end())
		{
			std::cout << "WARN: System with the name already exists: " << systemName << std::endl;
			return nullptr;
		}

		auto id = Systems.size();
		auto system = std::make_unique<SystemClass>(SystemClass(args...));

		system->ID = id;
		system->AllowThreading = allowThreading;
		system->WorldComponentManager = WorldComponentManager;
		system->WorldEntityManager = WorldEntityManager;

		system->Init();


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

		WarnAboutMismatch(systemName, readOnlyIsEqual, WriteReadIsEqual, allowThreading, GetSetAsString(wantedReadOnly), GetSetAsString(wantedWriteRead));

		WorldComponentManager->FinishThreadingProtectionCheck();

		if ((allowThreading && readOnlyIsEqual && WriteReadIsEqual) == false)
		{
			return nullptr;
		}

		system->SetInitialized(true);
		system->SetEnabled(true);

		auto returnSystem = system.get();

		Systems.push_back(std::move(system));
		std::type_index systemTypeIndex = typeid(SystemClass);
		SystemNames.insert(std::pair(systemName, id));
		SystemIDs.insert(std::pair(systemTypeIndex, id));

		return returnSystem;
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

	System* GetSystemById(SystemID systemId)
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

	System* GetSystemByName(std::string systemName)
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

protected:
	void UpdateSystems(float deltaTime)
	{
		//PreUpdate
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::System)
			{
				Systems.at(i)->PreUpdate(deltaTime);
			}
		}

		//Update
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::System)
			{
				Systems.at(i)->SystemUpdate(deltaTime);
			}
		}

		//Update
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::System)
			{
				Systems.at(i)->PostUpdate(deltaTime);
			}
		}
	}

	void UpdateRenderSystems(float deltaTime)
	{
		//PreUpdate
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::RenderSystem)
			{
				Systems.at(i)->PreUpdate(deltaTime);
			}
		}
		//Update
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::RenderSystem)
			{
				Systems.at(i)->SystemUpdate(deltaTime);
			}
		}

		//Update
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
		//PreUpdate
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::PhysicsSystem)
			{
				Systems.at(i)->PreUpdate(deltaTime);
			}
		}
		//Update
		for (size_t i = 0; i < Systems.size(); i++)
		{
			if (Systems.at(i)->GetEnabled() && Systems.at(i)->GetType() == SystemType::PhysicsSystem)
			{
				Systems.at(i)->SystemUpdate(deltaTime);
			}
		}

		//Update
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


	void WarnAboutMismatch(std::string systemName, bool readOnlyIsEqual, bool WriteReadIsEqual, bool allowThreading, std::string wantedRO, std::string wantedRW)
	{
		if (!readOnlyIsEqual)
		{
			if (allowThreading)
			{
				fmt::println("CRITICAL ERROR when creating {}: WantedReadOnlyComponentTypes is different compared to what the system accesses", systemName);
				fmt::println("{} != {}", wantedRO, GetSetAsString(WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed));
			}
			else
			{
				fmt::println("Warning when creating {}: WantedReadOnlyComponentTypes is different compared to what the system accesses", systemName);
				fmt::println("{} != {}", wantedRO, GetSetAsString(WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed));
			}
		}
		if (!WriteReadIsEqual)
		{
			if (allowThreading)
			{
				fmt::println("CRITICAL ERROR when creating {}: WantedWriteReadComponentTypes is different compared to what the system accesses", systemName);
				fmt::println("{} != {}", wantedRW, GetSetAsString(WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed));
			}
			else
			{
				fmt::println("Warning when creating {}: WantedWriteReadComponentTypes is different compared to what the system accesses", systemName);
				fmt::println("{} != {}", wantedRW, GetSetAsString(WorldComponentManager->ThreadingProtectionReadOnlyComponentsAccessed));
			}
		}
	}


	EntityManager* WorldEntityManager;
	ComponentManager* WorldComponentManager;
	std::vector<std::unique_ptr<System>> Systems;
	std::vector<size_t> EntityComponentFilterQuerySystemIDs; //TODO: When EntityTypeStorage component list is modified, these systems need to re-evaluate what entities to process.
	std::map<std::string, SystemID> SystemNames;
	std::map<std::type_index, SystemID> SystemIDs;
};
