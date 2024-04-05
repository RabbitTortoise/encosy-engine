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
import <unordered_set>;
import <unordered_map>;
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
	EntityTypeStorage(const EntityType entityType)
	{
		StorageEntityType = entityType;
	}
	~EntityTypeStorage() {}

	EntityType GetStorageEntityType() const { return StorageEntityType; }
	const std::vector<std::type_index>& GetEntityComponentTypes() { return EntityComponentTypes; }

	void AddComponentType(const ComponentStorageLocator& locator, IComponentStorage* storagePointer)
	{
		ComponentStorageLocators.emplace_back(locator);
		EntityComponentTypes.emplace_back(locator.ComponentType);
		std::ranges::sort(ComponentStorageLocators, {}, &ComponentStorageLocator::ComponentType);
		std::ranges::sort(EntityComponentTypes, {});
		ComponentStoragePointers.emplace_back(std::make_pair(locator.ComponentType, storagePointer));
	}

	void AddEntityToStorage(const Entity entity)
	{
		EntityStorageLocator locator = { entity, EntityCount };
		EntityStorageLocations.emplace_back(locator);
		EntityCount++;
	}

	EntityComponentIndex GetEntityComponentIndex(const Entity entity)
	{
		auto it = std::ranges::find(EntityStorageLocations, entity, &EntityStorageLocator::Entity);
		if (it != EntityStorageLocations.end())
		{
			return it->ComponentIndex;
		}
		fmt::println("ERROR: Entity {} was not found from EntityStorageLocations, entityType {}", entity, StorageEntityType);
		return (-1);
	}

	Entity GetEntityFromComponentIndex(const EntityComponentIndex componentIndex)
	{
		auto it = std::ranges::find(EntityStorageLocations, componentIndex, &EntityStorageLocator::ComponentIndex);
		if (it != EntityStorageLocations.end())
		{
			return it->Entity;
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
		return std::ranges::find(ComponentStoragePointers, componentTypeId, &std::pair<std::type_index, IComponentStorage*>::first)->second;
	}

	IComponentStorage* GetComponentStoragePointer(const std::type_index componentTypeId) const 
	{
		return std::ranges::find(ComponentStoragePointers, componentTypeId, &std::pair<std::type_index, IComponentStorage*>::first)->second;
	}

	EntityStorageLocator GetEntityStorageLocator(const Entity entity)
	{
		auto it = std::ranges::find(EntityStorageLocations, entity, &EntityStorageLocator::Entity);
		if(it != EntityStorageLocations.end())
		{
			return  *it;
		}
		constexpr EntityStorageLocator locator = { static_cast<Entity>(-1) , static_cast<Entity>(-1) };
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

	bool ContainsEntity(const Entity entity) const
	{
		return (std::ranges::find(EntityStorageLocations, entity, &EntityStorageLocator::Entity) != EntityStorageLocations.end());
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
	std::vector<std::pair<std::type_index, IComponentStorage*>> ComponentStoragePointers;
	std::vector<EntityStorageLocator> EntityStorageLocations;
};
