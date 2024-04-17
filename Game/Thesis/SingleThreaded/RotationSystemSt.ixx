module;
#include <glm/glm.hpp>	

export module Thesis.Systems.ThesisRotationSystemSt;

import EncosyEngine.MatrixCalculations;
import EncosyCore.Entity;
import EncosyCore.System;
import Components.TransformComponent;
import Components.CameraComponent;
import Components.StaticComponent;
import Thesis.Components;

export class ThesisRotationSystemSt : public System
{
	friend class SystemManager;

public:
	ThesisRotationSystemSt() {}
	~ThesisRotationSystemSt() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;

		AddComponentQueryForReading(&RotatingEntityComponents);
		AddComponentQueryForWriting(&TransformComponents);

		AddForbiddenComponentQuery<CameraComponent>();
		AddForbiddenComponentQuery<StaticComponent>();
		AddForbiddenComponentQuery<DestroyEntityComponent>();
	}
	void PreUpdate(const double deltaTime) override {}
	void Update(const double deltaTime) override {}
	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override
	{
		TransformComponent& tc = GetCurrentEntityComponent(&TransformComponents);
		const RotateComponent rec = GetCurrentEntityComponent(&RotatingEntityComponents);
		
		tc.Orientation = MatrixCalculations::RotateByWorldAxisX(tc.Orientation, rec.Speed * rec.Direction.x * deltaTime + 20);
		tc.Orientation = MatrixCalculations::RotateByWorldAxisY(tc.Orientation, rec.Speed * rec.Direction.y * deltaTime + 20);
		tc.Orientation = MatrixCalculations::RotateByWorldAxisZ(tc.Orientation, rec.Speed * rec.Direction.z * deltaTime + 20);
	}

	void PostUpdate(const double deltaTime) override {}
	void Destroy() override {}

private:
	
	ReadOnlyComponentStorage<RotateComponent> RotatingEntityComponents;
	WriteReadComponentStorage<TransformComponent> TransformComponents;
};
