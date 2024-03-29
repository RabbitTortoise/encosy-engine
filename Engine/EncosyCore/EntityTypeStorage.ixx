module;

#include <fmt/format.h>
#include <algorithm>

export module EncosyCore.EntityTypeStorage;

import EncosyCore.ComponentTypeStorage;
import EncosyCore.Entity;

import <vector>;
import <string>;
import <map>;
import <span>;
import <typeindex>;
import <typeinfo>;
import <memory>;


export typedef size_t EntityComponentIndex;

export struct EntityStorageLocator
{                                 
	Entity Entity;
	EntityComponentIndex ComponentIndex;
};

export class EntityTypeStorage
{

public:
	EntityTypeStorage(EntityType entityType) 
	{
		StorageEntityType = entityType;
	}
	~EntityTypeStorage() {}

	EntityType GetStorageEntityType() const { return StorageEntityType; }
	const std::vector<std::type_index>& GetEntityComponentTypes() { return EntityComponentTypes; }

	void AddComponentType(const ComponentStorageLocator locator, IComponentStorage* storagePointer)
	{
		ComponentStorageLocators.push_back({ locator });
		EntityComponentTypes.push_back(locator.ComponentType);
		std::ranges::sort(ComponentStorageLocators, {}, &ComponentStorageLocator::ComponentType);
		std::ranges::sort(EntityComponentTypes, {});
		ComponentStoragePointers.insert(std::make_pair(locator.ComponentType, storagePointer));
	}

	void AddEntityToStorage(const Entity entity)
	{
		EntityStorageLocations.push_back({ entity, EntityCount });
		EntityCount++;
	}

	EntityComponentIndex GetEntityComponentIndex(const Entity entity)
	{
		if (std::ranges::find(EntityStorageLocations, entity, &EntityStorageLocator::Entity) != EntityStorageLocations.end())
		{
			return std::ranges::find(EntityStorageLocations, entity, &EntityStorageLocator::Entity)->ComponentIndex;
		}
		fmt::println("ERROR: Entity {} was not found from EntityStorageLocations, entityType {}", entity, StorageEntityType);
		return (-1);
	}

	Entity GetEntityFromComponentIndex(EntityComponentIndex componentIndex)
	{
		if (std::ranges::find(EntityStorageLocations, componentIndex, &EntityStorageLocator::ComponentIndex) != EntityStorageLocations.end())
		{
			return std::ranges::find(EntityStorageLocations, componentIndex, &EntityStorageLocator::ComponentIndex)->Entity;
		}
		fmt::println("ERROR: componentIndex {} was not found from EntityStorageLocations, entityType {}", componentIndex, StorageEntityType);
		return -1;
	}

	template<typename ComponentType>
	ComponentStorageLocator GetComponentStorageLocator() const
	{
		const std::type_index componentTypeId = typeid(ComponentType);
		return ComponentStorageLocators[GetComponentTypeVectorIndex(componentTypeId)];
	}

	template<typename ComponentType>
	IComponentStorage* GetComponentStoragePointer() const
	{
		const std::type_index componentTypeId = typeid(ComponentType);
		return ComponentStoragePointers.find(componentTypeId)->second;
	}

	IComponentStorage* GetComponentStoragePointer(std::type_index componentTypeId) const 
	{
		return ComponentStoragePointers.find(componentTypeId)->second;
	}


	EntityStorageLocator GetEntityStorageLocator(const Entity entity)
	{
		if(std::ranges::find(EntityStorageLocations, entity, &EntityStorageLocator::Entity) != EntityStorageLocations.end())
		{
			return  *std::ranges::find(EntityStorageLocations, entity, &EntityStorageLocator::Entity);
		}
		constexpr  EntityStorageLocator locator = { static_cast<Entity>(-1) , static_cast<Entity>(-1) };
		return locator;
	}

	std::span<EntityStorageLocator const> GetEntityStorageLocators()
	{
		return EntityStorageLocations;
	}

	std::span<ComponentStorageLocator const> GetComponentStorageLocators()
	{
		return ComponentStorageLocators;
	}

	template<typename ComponentType>
	bool HasComponentType() const
	{
		if(std::ranges::find(EntityComponentTypes, typeid(ComponentType)) != EntityComponentTypes.end())
		{
			return true;
		}
		return false;
	}

	bool RemoveEntity(const Entity entity)
	{
		const auto entityLocatorIt = std::ranges::find(EntityStorageLocations, entity, &EntityStorageLocator::Entity);

		if(entityLocatorIt != EntityStorageLocations.end())
		{
			const EntityComponentIndex newIndex = entityLocatorIt->ComponentIndex;
			const auto lastIt = EntityStorageLocations.end() - 1;
			*entityLocatorIt = std::move(*lastIt);
			entityLocatorIt->ComponentIndex = newIndex;
			EntityStorageLocations.pop_back();
			EntityCount--;
			return true;
		}
		return false;
	}

	size_t GetEntityCount() const { return EntityCount; }

private:

	size_t GetComponentTypeVectorIndex(const std::type_index componentTypeId) const
	{
		size_t index = 0;
		for (auto element : EntityComponentTypes)
		{
			if (componentTypeId == element)
			{
				return index;
			}
			index++;
		}
		return -1;
	}

	size_t EntityCount = 0;
	EntityType StorageEntityType;
	std::vector<std::type_index> EntityComponentTypes;
	std::vector<ComponentStorageLocator> ComponentStorageLocators;
	std::map<std::type_index, IComponentStorage*> ComponentStoragePointers;
	std::vector<EntityStorageLocator> EntityStorageLocations;
};
