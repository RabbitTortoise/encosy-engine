module;
export module EncosyCore.EncosyWorld;

//Core Managers
export import ECS.EntityManager;
export import ECS.ComponentManager;
export import ECS.SystemManager;

//Core Types
export import ECS.Entity;
export import ECS.System;

//Core Data Storage
export import ECS.ComponentTypeStorage;
export import ECS.EntityTypeStorage;

import <memory>;
import <vector>;

export class EncosyWorld
{
public:
	EncosyWorld()
	{
		WorldComponentManager = std::make_unique<ComponentManager>();
		WorldEntityManager = std::make_unique<EntityManager>(WorldComponentManager.get());
		WorldSystemManager = std::make_unique<SystemManager>(WorldComponentManager.get(), WorldEntityManager.get());
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
	std::unique_ptr<ComponentManager> WorldComponentManager;
	std::unique_ptr<EntityManager> WorldEntityManager;
	std::unique_ptr<SystemManager> WorldSystemManager;

};