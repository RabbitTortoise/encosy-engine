module;
export module ECS.ComponentTypeStorage;

import <vector>;
import <string>;
import <utility>;
import <memory>;
import <typeindex>;
import <typeinfo>;
import <span>;

export class IComponentStorage
{
	friend class ComponentManager;

public:
	IComponentStorage() {}
	~IComponentStorage() {}

	virtual size_t GetComponentCount() = 0;
};


export struct ComponentStorageLocator
{
public:
	std::type_index ComponentType = typeid(ComponentStorageLocator);
	size_t ComponentStorageIndex = -1;
};


/// <summary>
/// This is used to get to store one type of Component. It has methods for adding, removing and getting a single component.
/// </summary>
/// <typeparam name="T"></typeparam>
export template <typename ComponentType>
class ComponentTypeStorage: public IComponentStorage
{
	friend class ComponentManager;
	friend class EntityTypeStorage;

public:
	ComponentTypeStorage()
	{
		Storage = std::vector<ComponentType>();
		Storage.reserve(1);
	}
	~ComponentTypeStorage(){}
	
	size_t GetComponentCount()
	{
		return Storage.size();
	}

protected:
	/// <summary>
	/// Adds a new Component to the vector.
	/// </summary>
	/// <returns></returns>
	void AddComponentToStorage(const ComponentType component)
	{
		Storage.push_back(component);
	}

	ComponentType* GetComponent(const size_t id)
	{
		if (id < GetComponentCount())
		{
			return &Storage->at(id);
		}
		return nullptr;
	}

	/// <summary>
	/// Removes component from specific index
	/// </summary>
	/// <param name="index"></param>
	/// <returns></returns>
	bool RemoveComponent(const size_t index)
	{
		if (index < GetComponentCount())
		{
			auto removedIt = Storage.begin() + index;
			auto lastIT = Storage.back();
			*removedIt = std::move(lastIT);
			Storage.pop_back();
			return true;
		}
		return false;
	}

	void SetComponentData(const size_t index, const ComponentType component)
	{
		Storage[index] = component;
	}

	void Reset()
	{
		Storage.clear();
	}

	const std::span<ComponentType const> GetStorageReadOnlySpan()
	{
		return std::span<ComponentType const>(Storage);
	}

	std::span<ComponentType> GetStorageWriteReadSpan()
	{
		return std::span<ComponentType>(Storage);
	}
	
private:
	std::vector<ComponentType> Storage;
};



export template <typename ComponentType>
class SystemDataStorage : public IComponentStorage
{
	friend class ComponentManager;
	friend class EntityTypeStorage;

public:
	SystemDataStorage(){}
	~SystemDataStorage() {}

	size_t GetComponentCount()
	{
		return 1;
	}

protected:

	void SetComponentData(const ComponentType component)
	{
		SystemDataComponent = component;
	}

	ComponentType GetReadOnly()
	{
		return SystemDataComponent;
	}

	ComponentType* GetWriteRead()
	{
		return &SystemDataComponent;
	}


private:


	ComponentType SystemDataComponent;
};