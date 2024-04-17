module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Thesis.Systems.ThesisModelMatrixBuilderSystemMt;

import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import Components.TransformComponent;
import Components.ModelMatrixComponent;
import Components.StaticComponent;
import EncosyEngine.MatrixCalculations;

import <map>;
import <span>;
import <vector>;
import <iostream>;

export class ThesisModelMatrixBuilderSystemMt : public SystemThreaded
{
	friend class SystemManager;

	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = false,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = false,
	.IgnoreThreadSaveFunctions = false,
	};


public:
	ThesisModelMatrixBuilderSystemMt(bool allowConcurrent)
	{
		ThreadedRunOptions.PreferRunAlone = !allowConcurrent;
	}
	~ThesisModelMatrixBuilderSystemMt() {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunAfterSpecificSystem = "ThesisRotationSystemMt";
		SetThreadedRunOptions(ThreadedRunOptions);

		AddComponentQueryForReading(&TransformComponents);
		AddComponentQueryForWriting(&ModelMatrixComponents, &ThreadModelMatrixComponents);
		AddForbiddenComponentQuery<StaticComponent>();

	}
	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override {}
	void UpdatePerEntity(const int thread, const double deltaTime, const Entity entity, const EntityType entityType) override
	{
		const TransformComponent tc = GetCurrentEntityComponent(thread, &TransformComponents);
		ModelMatrixComponent& mc = GetCurrentEntityComponent(thread, &ThreadModelMatrixComponents);
		mc.ModelMatrix = MatrixCalculations::CalculateModelMatrix(tc);
	}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:
	ReadOnlyComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<ModelMatrixComponent> ModelMatrixComponents;

	ThreadComponentStorage<ModelMatrixComponent> ThreadModelMatrixComponents;
};
