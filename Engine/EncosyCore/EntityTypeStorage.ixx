module;

#include <algorithm>

export module ECS.EntityTypeStorage;

import ECS.ComponentTypeStorage;
import ECS.Entity;

import <vector>;
import <string>;
import <map>;
import <typeindex>;
import <typeinfo>;
import <memory>;
import <unordered_map>;


export typedef size_t EntityComponentIndex;

export struct EntityStorageLocator
{
	EntityID ID;
	EntityComponentIndex ComponentIndex;
};

export class EntityTypeStorage
{
	friend class EntityManager;

public:
	EntityTypeStorage(EntityTypeID entityTypeId) 
	{
		StorageEntityTypeID = entityTypeId;
	}
	~EntityTypeStorage() {}

	EntityTypeID GetStorageEntityTypeID() const { return StorageEntityTypeID; }
	std::vector<std::type_index> GetEntityComponentTypes() const { return EntityComponentTypes; }

protected:
	template<typename ComponentType>
	void AddComponentType(const ComponentStorageLocator locator)
	{
		std::type_index componentTypeId = typeid(ComponentType);
		ComponentStorageLocators.insert(std::pair(componentTypeId, locator));
		EntityComponentTypes.push_back(componentTypeId);
		std::sort(EntityComponentTypes.begin(), EntityComponentTypes.end());
	}

	void AddEntityToStorage(const EntityID entityId)
	{
		StorageLocations.push_back({ entityId, EntityCount });
		EntityCount++;
	}

	const EntityComponentIndex GetEntityStorageID(const EntityID entityId)
	{
		auto entityStorageId = std::ranges::find_if(StorageLocations, [=](const auto& location) {
			return location.ID == entityId;
			});

		if (entityStorageId == StorageLocations.end())
		{
			return -1;
		}
		return entityStorageId->ComponentIndex;
	}

	const EntityID GetEntityIdFromComponentIndex(EntityComponentIndex componentIndex)
	{
		auto it = std::ranges::find_if(StorageLocations, [=](const auto& location) {
			return location.ComponentIndex == componentIndex;
			});
		if (it != StorageLocations.end())
		{
			return it->ComponentIndex;
		}
		return -1;
	}

	template<typename ComponentType>
	const ComponentStorageLocator GetComponentStorageLocator()
	{
		std::type_index componentTypeId = typeid(ComponentType);
		auto it = ComponentStorageLocators.find(componentTypeId);
		if (it == ComponentStorageLocators.end())
		{
			abort();
		}
		return it->second;
	}
	
	const std::vector<EntityStorageLocator> GetStorageLocators()
	{
		return StorageLocations;
	}


private:
	size_t EntityCount = 0;
	EntityTypeID StorageEntityTypeID;
	std::vector<std::type_index> EntityComponentTypes;
	std::vector<EntityStorageLocator> StorageLocations;
	std::map<std::type_index, ComponentStorageLocator> ComponentStorageLocators;
};
