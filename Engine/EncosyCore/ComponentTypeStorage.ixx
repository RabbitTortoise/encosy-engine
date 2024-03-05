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
	virtual ~IComponentStorage() {}

	virtual size_t GetComponentCount() = 0;
	virtual bool RemoveComponent(const size_t index) = 0;
	virtual bool CopyComponentToOtherStorage(const size_t id, IComponentStorage* otherStorage) = 0;
	virtual std::unique_ptr<IComponentStorage> InitializeSimilarStorage() = 0;
	virtual void Reset() = 0;
	virtual void AddComponentToStorage() = 0;
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
	~ComponentTypeStorage() override {}
	
	size_t GetComponentCount() override
	{
		return Storage.size();
	}

	/// <summary>
	/// Removes component from specific index
	/// </summary>
	/// <param name="index"></param>
	/// <returns></returns>
	bool RemoveComponent(const size_t index) override
	{
		if (index < GetComponentCount())
		{
			auto removedIt = Storage.begin() + index;
			auto lastIT = Storage.end() - 1;
			*removedIt = std::move(*lastIT);
			Storage.pop_back();
			return true;
		}
		return false;
	}

	bool CopyComponentToOtherStorage(const size_t id, IComponentStorage* otherStorage) override
	{
		auto storage = static_cast<ComponentTypeStorage<ComponentType>*>(otherStorage);
		if (id < GetComponentCount())
		{
			storage->AddComponentToStorage(Storage[id]);
			return true;
		}
		return false;
	}

	std::unique_ptr<IComponentStorage> InitializeSimilarStorage() override
	{
		auto newStorage = std::make_unique<ComponentTypeStorage<ComponentType>>();
		return std::move(newStorage);
	}

	void Reset() override
	{
		Storage.clear();
	}

	void AddComponentToStorage() override
	{
		Storage.push_back(ComponentType());
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

	ComponentType* GetWriteReadComponent(const size_t id)
	{
		if (id < GetComponentCount())
		{
			return &Storage[id];
		}
		return nullptr;
	}

	ComponentType GetReadOnlyComponent(const size_t id)
	{
		if (id < GetComponentCount())
		{
			return Storage[id];
		}
		return {};
	}

	void SetComponentData(const size_t index, const ComponentType component)
	{
		Storage[index] = component;
	}

	std::span<ComponentType const> GetStorageReadOnlySpan() const
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
	SystemDataStorage()
	{
		SystemDataComponent.reserve(1);
		SystemDataComponent.push_back(ComponentType{});
	}
	~SystemDataStorage() override {}

	size_t GetComponentCount() override
	{
		return 1;
	}

	void AddComponentToStorage() override {}

	std::unique_ptr<IComponentStorage> InitializeSimilarStorage() override
	{
		auto newStorage = std::make_unique<SystemDataStorage<ComponentType>>();
		return std::move(newStorage);
	}

protected:

	void SetComponentData(const ComponentType component)
	{
		SystemDataComponent[0] = component;
	}

	std::span<ComponentType const> GetReadOnly()
	{
		return SystemDataComponent;
	}

	std::span<ComponentType> GetWriteRead()
	{
		return SystemDataComponent;
	}

	void Reset() override
	{
		SystemDataComponent.clear();
	}

	bool RemoveComponent(const size_t index) override {return false;}
	bool CopyComponentToOtherStorage(const size_t id, IComponentStorage* otherStorage) override
	{
		auto storage = static_cast<SystemDataStorage<ComponentType>*>(otherStorage);
		storage->SetComponentData(SystemDataComponent[0]);
		return true;
	}

private:
	std::vector<ComponentType> SystemDataComponent;
};