module;
#include <fmt/core.h>

export module Thesis.Systems.ThesisEntityModifySystemSt1;

import EncosyCore.Entity;
import EncosyCore.System;
import RenderCore.MeshLoader;
import RenderCore.VulkanTypes;

import Components.TransformComponent;
import Components.MaterialComponent;
import Thesis.Components;

import <vector>;
import <array>;


export class ThesisEntityModifySystemSt1 : public System
{
	friend class SystemManager;

public:

	ThesisEntityModifySystemSt1() {}
	~ThesisEntityModifySystemSt1() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;

		AddRequiredComponentQuery<EntityTypeComponent1>();
		AddForbiddenComponentQuery<DestroyEntityComponent>();

		RunWithSpecificSystem = "ThesisEntityModifySystemSt1";

		EnableDestructiveAccessToEntityStorage(GetEntityTypeInfo("EntityType1").Type);
		EnableDestructiveAccessToEntityStorage(GetEntityTypeInfo("EntityToDestroy1").Type);

	}

	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override {}

	void UpdatePerEntity(const double deltaTime, const Entity entity, const EntityType entityType) override
	{
		DestroyEntityComponent dec = {};
		ModifyEntityComponents<RotateComponent>(entity, entityType, dec);
}

	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:

};
