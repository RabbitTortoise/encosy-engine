module;
#include <glm/glm.hpp>	

export module Thesis.Systems.ThesisEntityDestroySystemSt1;

import EncosyCore.Entity;
import EncosyCore.System;
import Thesis.Components;


export class ThesisEntityDestroySystemSt1 : public System
{

	friend class SystemManager;

public:
	ThesisEntityDestroySystemSt1() {}
	~ThesisEntityDestroySystemSt1() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunAfterSpecificSystem = "ThesisEntitySpawningSystemSt10";
		RunWithSpecificSystem = "ThesisEntityDestroySystemSt1";
		RunBeforeSpecificSystem = "ThesisRotationSystemSt";

		AddRequiredComponentQuery<EntityTypeComponent1>();
		AddRequiredComponentQuery<DestroyEntityComponent>();

		EnableDestructiveAccessToEntityStorage(GetEntityTypeInfo("EntityToDestroy1").Type);

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
