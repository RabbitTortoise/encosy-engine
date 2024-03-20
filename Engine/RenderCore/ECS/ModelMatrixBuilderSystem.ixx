module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module Systems.ModelMatrixBuilderSystem;

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

export class ModelMatrixBuilderSystem : public SystemThreaded
{
	friend class SystemManager;

public:
	ModelMatrixBuilderSystem() {}
	~ModelMatrixBuilderSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::RenderSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;

		AddComponentQueryForReading(&TransformComponents);
		AddComponentQueryForWriting(&ModelMatrixComponents, &ThreadModelMatrixComponents);
		AddForbiddenComponentQuery<StaticComponent>();

	}
	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override {}
	void UpdatePerEntity(int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		const TransformComponent tc = GetCurrentEntityComponent(thread, &TransformComponents);
		ModelMatrixComponent& mc = GetCurrentEntityComponent(thread, &ThreadModelMatrixComponents);
		mc.ModelMatrix = MatrixCalculations::CalculateModelMatrix(tc);
	}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:

	EntityType cameraType = -1;

	ReadOnlyComponentStorage<TransformComponent> TransformComponents;
	WriteReadComponentStorage<ModelMatrixComponent> ModelMatrixComponents;

	ThreadComponentStorage<ModelMatrixComponent> ThreadModelMatrixComponents;
};
