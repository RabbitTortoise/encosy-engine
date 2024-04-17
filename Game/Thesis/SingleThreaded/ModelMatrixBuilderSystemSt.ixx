module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Thesis.Systems.ThesisModelMatrixBuilderSystemSt;

import EncosyCore.Entity;
import EncosyCore.System;
import Components.TransformComponent;
import Components.ModelMatrixComponent;
import Components.StaticComponent;
import EncosyEngine.MatrixCalculations;

import <map>;
import <span>;
import <vector>;
import <iostream>;

export class ThesisModelMatrixBuilderSystemSt : public System
{
	friend class SystemManager;

public:
	ThesisModelMatrixBuilderSystemSt() {}
	~ThesisModelMatrixBuilderSystemSt() {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunAfterSpecificSystem = "ThesisRotationSystemSt";

		AddComponentQueryForReading(&TransformComponents);
		AddComponentQueryForWriting(&ModelMatrixComponents);
		AddForbiddenComponentQuery<StaticComponent>();

	}
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override {}
	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override
	{
		const TransformComponent tc = GetCurrentEntityComponent(&TransformComponents);
		ModelMatrixComponent& mc = GetCurrentEntityComponent(&ModelMatrixComponents);
		mc.ModelMatrix = MatrixCalculations::CalculateModelMatrix(tc);
	}

	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:
	ReadOnlyComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<ModelMatrixComponent> ModelMatrixComponents;

};
