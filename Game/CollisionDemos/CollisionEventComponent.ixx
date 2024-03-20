module;

export module Demo.Components.CollisionEventComponent;

import EncosyCore.Entity;


export struct CollisionEventComponent
{
	Entity A;
	Entity B;
	float CollisionDepth;
};