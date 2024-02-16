module;
export module ECS.Entity;

import <utility>;
import <string>;

export typedef unsigned int EntityID;
export typedef unsigned int EntityTypeID;

export class Entity
{
	friend class EntityManager;

public:
	Entity() { ID = -1; Type = -1;}
	Entity(EntityID entityID, EntityTypeID entityTypeID) : ID(entityID), Type(entityTypeID) { }
	Entity(Entity const& e) : ID(e.ID), Type(e.Type) {}
	Entity(Entity&& e) noexcept : ID(std::move(e.ID)), Type(std::move(e.Type)) {}
	Entity& operator=(const Entity& e) { ID = e.ID; return *this; }
	~Entity() {}

	EntityID GetID() const { return ID; }
	EntityID GetType() const { return Type; }

protected:
	void SetEntityID(EntityID id) { ID = id; }
	void SetEntityType(EntityTypeID type) { Type = type; }

private:
	EntityID ID;
	EntityTypeID Type;
};

export class EntityTypeInfo
{
public:
	enum class Message { TypeFound, NotFound, Created, Exists, Error };

	EntityTypeInfo()
	{
		Message = Message::NotFound;
		TypeID = -1;
		TypeName = "DOES NOT EXIST";
	}
	EntityTypeInfo(Message message, EntityTypeID typeId, std::string typeName)
	{
		Message = message;
		TypeID = typeId;
		TypeName = typeName;
	}
	Message Message;
	EntityTypeID TypeID;
	std::string TypeName;

	std::string GetMessageAsString()
	{
		switch (Message)
		{
		case EntityTypeInfo::Message::TypeFound:
			return "TypeFound";
			break;
		case EntityTypeInfo::Message::NotFound:
			return "NotFound";
			break;
		case EntityTypeInfo::Message::Created:
			return "Created";
			break;
		case EntityTypeInfo::Message::Exists:
			return "Exists";
			break;
		case EntityTypeInfo::Message::Error:
			return "Error";
			break;
		default:
			break;
		}
	}
};
