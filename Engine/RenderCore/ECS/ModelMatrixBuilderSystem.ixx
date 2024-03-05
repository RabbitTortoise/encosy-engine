module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Systems.ModelMatrixBuilderSystem;

import ECS.Entity;
import ECS.System;
import Components.TransformComponent;
import Components.ModelMatrixComponent;
import Components.StaticComponent;
import EncosyEngine.MatrixCalculations;

import <map>;
import <span>;
import <vector>;
import <iostream>;

export class ModelMatrixBuilderSystem : public ThreadedSystem
{
	friend class SystemManager;

public:
	ModelMatrixBuilderSystem() {}
	~ModelMatrixBuilderSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::RenderSystem;
		SystemQueueIndex = 10000;

		AddWantedComponentDataForReading(&TransformComponents);
		AddWantedComponentDataForWriting(&ModelMatrixComponents, &ThreadModelMatrixComponents);
		AddForbiddenComponentType<StaticComponent>();

	}
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override {}
	void UpdatePerEntityThreaded(int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		const TransformComponent tc = GetCurrentEntityComponent(thread, &TransformComponents);
		ModelMatrixComponent& mc = GetCurrentEntityComponent(thread, &ThreadModelMatrixComponents);
		mc.ModelMatrix = MatrixCalculations::CalculateModelMatrix(tc);
	}

	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:

	EntityType cameraType = -1;

	ReadOnlyComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<ModelMatrixComponent> ModelMatrixComponents;

	ThreadComponentStorage<ModelMatrixComponent> ThreadModelMatrixComponents;
};
