module;

export module Thesis.Systems.ThesisEntityModifySystemMt10;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import RenderCore.MeshLoader;
import RenderCore.VulkanTypes;

import Components.TransformComponent;
import Components.MaterialComponent;
import Thesis.Components;

import <vector>;
import <array>;


export class ThesisEntityModifySystemMt10 : public SystemThreaded
{

	struct EntityToChange
	{
		Entity entity;
		EntityType type;
	};

	friend class SystemManager;

	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = true,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = true,
	.IgnoreThreadSaveFunctions = true,
	};

public:

	ThesisEntityModifySystemMt10() {}
	~ThesisEntityModifySystemMt10() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		SetThreadedRunOptions(ThreadedRunOptions);

		AddRequiredComponentQuery<EntityTypeComponent1>();
		AddForbiddenComponentQuery<DestroyEntityComponent>();

		EnableDestructiveAccessToEntityStorage(GetEntityTypeInfo("EntityType1").Type);
		EnableDestructiveAccessToEntityStorage(GetEntityTypeInfo("EntityToDestroy1").Type);

	}

	void PreUpdate(const int thread, const double deltaTime) override{}
	void Update(const int thread, const double deltaTime) override {}

	void UpdatePerEntity(const int thread, const double deltaTime, const Entity entity, const EntityType entityType) override
	{
		DestroyEntityComponent dec;
		ModifyEntityComponents<RotateComponent>(entity, entityType, dec);
	}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}


private:

};
