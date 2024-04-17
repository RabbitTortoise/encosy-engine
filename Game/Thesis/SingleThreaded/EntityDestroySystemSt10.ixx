module;
#include <glm/glm.hpp>	

export module Thesis.Systems.ThesisEntityDestroySystemSt10;

import EncosyCore.Entity;
import EncosyCore.System;

import Thesis.Components;


export class ThesisEntityDestroySystemSt10 : public System
{

	friend class SystemManager;

public:
	ThesisEntityDestroySystemSt10() {}
	~ThesisEntityDestroySystemSt10() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunAfterSpecificSystem = "ThesisEntitySpawningSystemSt10";
		RunWithSpecificSystem = "ThesisEntityDestroySystemSt1";
		RunBeforeSpecificSystem = "ThesisRotationSystemSt";

		AddRequiredComponentQuery<EntityTypeComponent10>();
		AddRequiredComponentQuery<DestroyEntityComponent>();

		EnableDestructiveAccessToEntityStorage(GetEntityTypeInfo("EntityToDestroy10").Type);

	}
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override {}
	void UpdatePerEntity(const double deltaTime, const Entity entity, const EntityType entityType) override
	{
		bool success = DeleteEntity(entity, entityType);
	}
	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:
};
