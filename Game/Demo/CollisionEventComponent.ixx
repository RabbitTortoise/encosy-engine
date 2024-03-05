module;

export module Demo.Components.CollisionEventComponent;

import ECS.Entity;


export struct CollisionEventComponent
{
	Entity A;
	Entity B;
	float CollisionDepth;
};