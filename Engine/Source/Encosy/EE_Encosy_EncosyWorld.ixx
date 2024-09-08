module;
export module EE_Encosy_EncosyWorld;

//Core Managers
export import EE_Encosy_EntityManager;
export import EE_Encosy_ComponentManager;
export import EE_Encosy_SystemManager;
export import EE_Encosy_SharedBetweenManagers;

//Core Types
export import EE_Encosy_Entity;
export import EE_Encosy_System;

//Core Data Storage
export import EE_Encosy_ComponentTypeStorage;
export import EE_Encosy_EntityTypeStorage;

import <memory>;
import <vector>;

export class EncosyWorld
{
public:
	EncosyWorld()
	{
		WorldSharedBetweenManagers = std::make_unique<SharedBetweenManagers>();
		WorldComponentManager = std::make_unique<ComponentManager>(WorldSharedBetweenManagers.get());
		WorldEntityManager = std::make_unique<EntityManager>(WorldSharedBetweenManagers.get(), WorldComponentManager.get());
		WorldSystemManager = std::make_unique<SystemManager>(WorldSharedBetweenManagers.get(), WorldComponentManager.get(), WorldEntityManager.get());
	}
	~EncosyWorld()
	{
		WorldSystemManager.reset();
		WorldEntityManager.reset();
		WorldComponentManager.reset();
	}

	ComponentManager* GetWorldComponentManager() { return WorldComponentManager.get(); }
	EntityManager* GetWorldEntityManager() { return WorldEntityManager.get(); }
	SystemManager* GetWorldSystemManager() { return WorldSystemManager.get(); }

private:
	std::unique_ptr<SharedBetweenManagers> WorldSharedBetweenManagers;
	std::unique_ptr<ComponentManager> WorldComponentManager;
	std::unique_ptr<EntityManager> WorldEntityManager;
	std::unique_ptr<SystemManager> WorldSystemManager;

};