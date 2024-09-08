module;
#include <glm/glm.hpp>	
#include <glm/gtc/quaternion.hpp>	
#include <fmt/core.h>

export module EE_ECS_Systems_ModelMatrixBuilderSystem;

import EE_Encosy_Entity;
import EE_Encosy_SystemThreaded;
import EE_ECS_Components_TransformComponent;
import EE_ECS_Components_ModelMatrixComponent;
import EE_ECS_Components_StaticComponent;
import EE_Core_MatrixCalculations;

import <map>;
import <span>;
import <vector>;
import <iostream>;

export class EE_ModelMatrixBuilderSystem : public SystemThreaded
{

public:
	EE_ModelMatrixBuilderSystem() {}
	~EE_ModelMatrixBuilderSystem() {}

	void Init() override
	{
		Type = SystemType::RenderSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;

		AddComponentQueryForReading(&TransformComponents);
		AddComponentQueryForWriting(&ModelMatrixComponents, &ThreadModelMatrixComponents);
		AddForbiddenComponentQuery<EE_StaticComponent>();

	}
	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override {}
	void UpdatePerEntity(int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		const EE_TransformComponent tc = GetCurrentEntityComponent(thread, &TransformComponents);
		EE_ModelMatrixComponent& mc = GetCurrentEntityComponent(thread, &ThreadModelMatrixComponents);
		mc.ModelMatrix = MatrixCalculations::CalculateModelMatrix(tc);
	}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:
	ReadOnlyComponentStorage<EE_TransformComponent> TransformComponents;
	WriteReadComponentStorage<EE_ModelMatrixComponent> ModelMatrixComponents;

	ThreadComponentStorage<EE_ModelMatrixComponent> ThreadModelMatrixComponents;
};
