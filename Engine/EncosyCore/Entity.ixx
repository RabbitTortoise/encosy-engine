module;
export module ECS.Entity;

import <utility>;
import <string>;

export typedef unsigned long long Entity;
export typedef unsigned long long EntityType;

//export class Entity
//{
//	friend class EntityManager;
//
//public:
//	Entity() { ID = -1;}
//	Entity(EntityID entityID) : ID(entityID) { }
//	Entity(Entity const& e) : ID(e.ID) {}
//	Entity(Entity&& e) noexcept : ID(std::move(e.ID)) {}
//	Entity& operator=(const Entity& e) { ID = e.ID; return *this; }
//	~Entity() {}
//
//	EntityID GetID() const { return ID; }
//
//protected:
//	void SetEntityID(EntityID id) { ID = id; }
//
//private:
//	EntityID ID;
//};
