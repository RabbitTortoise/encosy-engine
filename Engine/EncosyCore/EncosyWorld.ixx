module;
export module EncosyCore.EncosyWorld;

//Core Managers
export import EncosyCore.EntityManager;
export import EncosyCore.ComponentManager;
export import EncosyCore.SystemManager;
export import EncosyCore.SharedBetweenManagers;

//Core Types
export import EncosyCore.Entity;
export import EncosyCore.System;

//Core Data Storage
export import EncosyCore.ComponentTypeStorage;
export import EncosyCore.EntityTypeStorage;

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