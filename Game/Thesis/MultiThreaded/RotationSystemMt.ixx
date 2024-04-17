module;
#include <glm/glm.hpp>	

export module Thesis.Systems.ThesisRotationSystemMt;

import EncosyEngine.MatrixCalculations;
import EncosyCore.Entity;
import EncosyCore.SystemThreaded;
import Components.TransformComponent;
import Components.CameraComponent;
import Components.StaticComponent;
import Thesis.Components;

export class ThesisRotationSystemMt : public SystemThreaded
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
	ThesisRotationSystemMt(bool allowConcurrent)
	{
		ThreadedRunOptions.PreferRunAlone = !allowConcurrent;
	}
	~ThesisRotationSystemMt() override {}

protected:
	void Init() override
	{
		Type = SystemType::System;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		SetThreadedRunOptions(ThreadedRunOptions);

		AddComponentQueryForReading(&RotatingEntityComponents);
		AddComponentQueryForWriting(&TransformComponents, &ThreadTransformComponents);

		AddForbiddenComponentQuery<CameraComponent>();
		AddForbiddenComponentQuery<StaticComponent>();
		AddForbiddenComponentQuery<DestroyEntityComponent>();
	}
	void PreUpdate(const int thread, const double deltaTime) override {}
	void Update(const int thread, const double deltaTime) override {}
	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		TransformComponent& tc = GetCurrentEntityComponent(thread, &ThreadTransformComponents);
		const RotateComponent rec = GetCurrentEntityComponent(thread, &RotatingEntityComponents);

		tc.Orientation = MatrixCalculations::RotateByWorldAxisX(tc.Orientation, rec.Speed * rec.Direction.x * deltaTime + 20);
		tc.Orientation = MatrixCalculations::RotateByWorldAxisY(tc.Orientation, rec.Speed * rec.Direction.y * deltaTime + 20);
		tc.Orientation = MatrixCalculations::RotateByWorldAxisZ(tc.Orientation, rec.Speed * rec.Direction.z * deltaTime + 20);
	}

	void PostUpdate(const int thread, const double deltaTime) override {}
	void Destroy() override {}

private:
	
	ReadOnlyComponentStorage<RotateComponent> RotatingEntityComponents;
	WriteReadComponentStorage<TransformComponent> TransformComponents;
	ThreadComponentStorage<TransformComponent> ThreadTransformComponents;
};
