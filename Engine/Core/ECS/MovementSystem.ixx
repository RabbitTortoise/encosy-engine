module;

#include <glm/glm.hpp>	
#include <fmt/core.h>

export module Systems.MovementSystem;

import ECS.Entity;
import ECS.System;
import Components.TransformComponent;
import Components.MovementComponent;
import EncosyEngine.MatrixCalculations;

import <map>;
import <span>;
import <vector>;
import <iostream>;

export
class MovementSystem : public ThreadedSystem
{
	struct EntityData
	{
		std::vector<std::span<TransformComponent>> TransformComponents;
		std::vector<std::span<MovementComponent>> MovementComponents;
	};
	EntityData HandledData;

	friend class SystemManager;

public:
	MovementSystem() {}
	~MovementSystem() {}

protected:
	void Init() override 
	{
		Type = SystemType::System;
		SystemQueueIndex = 1100;

		AddWantedComponentDataForWriting(
			&HandledData.TransformComponents,
			&HandledData.MovementComponents
		);
	}
	void PreUpdate(float deltaTime) override {}
	void Update(float deltaTime) override {}
	void UpdatePerEntity(float deltaTime, Entity entity, size_t vectorIndex, size_t spanIndex) override
	{
		TransformComponent& transformComponent = HandledData.TransformComponents[vectorIndex][spanIndex];
		MovementComponent& movementComponent = HandledData.MovementComponents[vectorIndex][spanIndex];


		transformComponent.Orientation = MatrixCalculations::RotateByWorldAxisX(transformComponent.Orientation, 5.0f * deltaTime);
		transformComponent.Orientation = MatrixCalculations::RotateByWorldAxisY(transformComponent.Orientation, 5.0f * deltaTime);
		transformComponent.Orientation = MatrixCalculations::RotateByWorldAxisZ(transformComponent.Orientation, 5.0f * deltaTime);
	
	}

	void UpdatePerEntityTypeThreaded(float deltaTime, EntityTypeID typeID, size_t entityCount, size_t storageIndex) 
	{
		std::span<TransformComponent>& transformComponents = HandledData.TransformComponents[storageIndex];
		std::span<MovementComponent>& movementComponents = HandledData.MovementComponents[storageIndex];

		for (size_t i = 0; i < entityCount; i++)
		{
			TransformComponent& tc = transformComponents[i];

			//fmt::println("{}{}{}", tc.Position.x, tc.Position.y, tc.Position.z);
			//tc.Position.x += 0.1f * deltaTime;

		}
	}


	void ThreadedMovementPerEntity(EntityData* handledData, float deltaTime, size_t storageIndex, size_t firstIndex, size_t lastIndex)
	{
		std::vector<TransformComponent> copyTransformVector = CopyFromSpan(HandledData.TransformComponents[storageIndex]);
		std::vector<MovementComponent> copyMovementVector = CopyFromSpan(HandledData.MovementComponents[storageIndex]);

		for (size_t i = firstIndex; i < lastIndex; i++)
		{

		}
	}







	void PostUpdate(float deltaTime) override {}
	void Destroy() override {}

	
private:

	float RandomNumber0_1()
	{
		return (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
	}

};
