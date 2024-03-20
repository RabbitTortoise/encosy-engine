module;
export module EncosyCore.ComponentManager;

import EncosyCore.SharedBetweenManagers;
import EncosyCore.ComponentTypeStorage;
import Components.TransformComponent;

import <vector>;
import <map>;
import <iostream>;
import <typeindex>;
import <typeinfo>;
import <functional>;
import <memory>;
import <mutex>;
import <span>;
import <unordered_set>;

export enum class ComponentAccessType { ReadOnly = 0, WriteRead };

export class ComponentManager
{
public:

	friend class EntityManager;
	friend class SystemManager;

	ComponentManager(SharedBetweenManagers* sharedBetweenManagers) { WorldSharedBetweenManagers = sharedBetweenManagers; }
	~ComponentManager() {}

	template <typename ComponentType>
	ComponentStorageLocator CreateComponentStorage()
	{
		std::type_index componentTypeId = typeid(ComponentType);

		auto storageVectorIt = ComponentStorages.find(componentTypeId);
		if (storageVectorIt == ComponentStorages.end())
		{
			std::vector<std::unique_ptr<IComponentStorage>> storage;
			ComponentStorages.insert(std::pair(componentTypeId, std::move(storage)));
			storageVectorIt = ComponentStorages.find(componentTypeId);
		}
		auto storage = std::make_unique<ComponentTypeStorage<ComponentType>>();
		storageVectorIt->second.push_back(std::move(storage));

		return { componentTypeId, storageVectorIt->second.size() - 1 };
	}

	template<typename ComponentType>
	ComponentStorageLocator CreateSystemDataStorage(const ComponentType component)
	{
		std::type_index componentTypeId = typeid(ComponentType);

		auto storage = std::make_unique<SystemDataStorage<ComponentType>>();
		auto storagePtr = storage.get();

		SystemDataStorages.insert(std::make_pair(componentTypeId, std::move(storage)));
		storagePtr->SetComponentData(component);

		return { componentTypeId, SystemDataStorages.size() - 1 };
	}

	template<typename ComponentType>
	std::span<ComponentType const> GetReadOnlySystemData()
	{
		const std::type_index componentTypeId = typeid(ComponentType);
		RecordReadOnlyComponentAccess<ComponentType>();

		const auto it = SystemDataStorages.find(componentTypeId);
		std::unique_ptr<IComponentStorage> const& storage = it->second;
		SystemDataStorage<ComponentType>* storagePtr = static_cast<SystemDataStorage<ComponentType>*>(storage.get());

		return storagePtr->GetReadOnly();
	}

	template<typename ComponentType>
	std::span<ComponentType> GetWriteReadSystemData()
	{
		const std::type_index componentTypeId = typeid(ComponentType);
		RecordWriteReadComponentAccess<ComponentType>();

		const auto it = SystemDataStorages.find(componentTypeId);
		std::unique_ptr<IComponentStorage> const& storage = it->second;
		SystemDataStorage<ComponentType>* storagePtr = static_cast<SystemDataStorage<ComponentType>*>(storage.get());

		return storagePtr->GetWriteRead();
	}

	template<typename ComponentType>
	std::vector<std::span<ComponentType const>> GetReadOnlyComponentSpans()
	{
		const std::type_index componentTypeId = typeid(ComponentType);
		RecordReadOnlyComponentAccess<ComponentType>();
		std::vector<std::span<ComponentType const>> componentSpans;

		std::vector<std::unique_ptr<IComponentStorage>> const& storagesVector = ComponentStorages.find(componentTypeId)->second;
		for (auto const& storage : storagesVector)
		{
			ComponentTypeStorage<ComponentType>* storagePtr = static_cast<ComponentTypeStorage<ComponentType>*>(storage.get());
			componentSpans.push_back(storagePtr->GetStorageReadOnlySpan());
		}
		return componentSpans;
	}

	template<typename ComponentType>
	std::vector<std::span<ComponentType>> GetWriteReadComponentSpans()
	{
		const std::type_index componentTypeId = typeid(ComponentType);
		RecordWriteReadComponentAccess<ComponentType>();
		std::vector<std::span<ComponentType>> componentSpans;

		std::vector<std::unique_ptr<IComponentStorage>> const& storagesVector = ComponentStorages.find(componentTypeId)->second;
		for (auto const& storage : storagesVector)
		{
			ComponentTypeStorage<ComponentType>* storagePtr = static_cast<ComponentTypeStorage<ComponentType>*>(storage.get());
			componentSpans.push_back(storagePtr->GetStorageWriteReadSpan());
		}
		return componentSpans;
	}

	template<typename ComponentType>
	std::span<ComponentType const> GetReadOnlyStorageSpan(const ComponentStorageLocator locator)
	{
		RecordReadOnlyComponentAccess<ComponentType>();

		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		ComponentTypeStorage<ComponentType>* storagePtr = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		return storagePtr->GetStorageReadOnlySpan();
	}

	template<typename ComponentType>
	std::span<ComponentType const> GetReadOnlyStorageSpan(IComponentStorage* storageInterface)
	{
		RecordReadOnlyComponentAccess<ComponentType>();

		ComponentTypeStorage<ComponentType>* storagePtr = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		return storagePtr->GetStorageReadOnlySpan();
	}

	template<typename ComponentType>
	std::span<ComponentType> GetWriteReadStorageSpan(const ComponentStorageLocator locator)
	{
		RecordWriteReadComponentAccess<ComponentType>();

		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		ComponentTypeStorage<ComponentType>* storagePtr = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		return storagePtr->GetStorageWriteReadSpan();
	}

	template<typename ComponentType>
	std::span<ComponentType> GetWriteReadStorageSpan(IComponentStorage* storageInterface)
	{
		RecordWriteReadComponentAccess<ComponentType>();

		ComponentTypeStorage<ComponentType>* storagePtr = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		return storagePtr->GetStorageWriteReadSpan();
	}

	template<typename ComponentType>
	void RecordReadOnlyComponentAccess()
	{
		if (ThreadingProtectionCheckOngoing)
		{
			const std::type_index componentTypeId = typeid(ComponentType);
			ThreadingProtectionReadOnlyComponentsAccessed.insert(componentTypeId);
		}
	}

	template<typename ComponentType>
	void RecordWriteReadComponentAccess()
	{
		if (ThreadingProtectionCheckOngoing)
		{
			const std::type_index componentTypeId = typeid(ComponentType);
			ThreadingProtectionWriteReadComponentsAccessed.insert(componentTypeId);
		}
	}

	template<typename ComponentType>
	void CreateNewComponentToStorageThreaded(const ComponentStorageLocator locator, const ComponentType component)
	{
		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		auto storage = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		std::scoped_lock lock(DestructiveEditMutex);
		storage->AddComponentToStorage(component);
	}


	template<typename ComponentType>
	void CreateNewComponentToStorage(const ComponentStorageLocator locator, const ComponentType component)
	{
		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		auto storage = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		storage->AddComponentToStorage(component);
	}

	void CreateNewComponentToStorage(const ComponentStorageLocator locator) const
	{
		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		storageInterface->AddComponentToStorage();
	}

	void CreateNewComponentToStorage(IComponentStorage* storageInterface) const
	{
		storageInterface->AddComponentToStorage();
	}

	void ResetComponentStorage(const ComponentStorageLocator locator) const
	{
		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		storageInterface->Reset();
	}

	void ResetComponentStorageThreaded(const ComponentStorageLocator locator)
	{
		std::scoped_lock lock(DestructiveEditMutex);

		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		storageInterface->Reset();
	}

protected:


	template<typename ComponentType>
	void ReplaceComponentData(const ComponentStorageLocator locator, const size_t componentIndex, const ComponentType component)
	{
		//Maybe needs a check if index is valid?
		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		auto storage = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		storage->SetComponentData(componentIndex, component);
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromStorage(const ComponentStorageLocator locator, size_t index)
	{
		RecordReadOnlyComponentAccess<ComponentType>();

		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		auto storage = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		return storage->GetReadOnlyComponent(index);
	}

	template<typename ComponentType>
	ComponentType GetReadOnlyComponentFromStorage(IComponentStorage* storageInterface, size_t index)
	{
		RecordReadOnlyComponentAccess<ComponentType>();

		auto storage = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		return storage->GetReadOnlyComponent(index);
	}

	template<typename ComponentType>
	ComponentType* GetWriteReadComponentFromStorage(const ComponentStorageLocator locator, size_t index)
	{
		RecordWriteReadComponentAccess<ComponentType>();

		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		auto storage = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		return storage->GetWriteReadComponent(index);
	}

	template<typename ComponentType>
	ComponentType* GetWriteReadComponentFromStorage(IComponentStorage* storageInterface, size_t index)
	{
		RecordWriteReadComponentAccess<ComponentType>();

		auto storage = static_cast<ComponentTypeStorage<ComponentType>*>(storageInterface);
		return storage->GetWriteReadComponent(index);
	}

	bool RemoveComponent(const ComponentStorageLocator locator, const size_t componentIndex) const
	{
		IComponentStorage* storageInterface = ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
		return storageInterface->RemoveComponent(componentIndex);
	}

	bool RemoveComponent(IComponentStorage* storageInterface, const size_t componentIndex) const
	{
		return storageInterface->RemoveComponent(componentIndex);
	}

	IComponentStorage* GetComponentStoragePointer(const ComponentStorageLocator locator)
	{
		return ComponentStorages.find(locator.ComponentType)->second[locator.ComponentStorageIndex].get();
	}

	ComponentStorageLocator CreateSimilarStorage(const std::type_index componentType)
	{
		auto& storagesVector = ComponentStorages.find(componentType)->second;
		auto newStorage = storagesVector[0]->InitializeSimilarStorage();
		storagesVector.push_back(std::move(newStorage));
		return { componentType, storagesVector.size() - 1 };;
	}

	ComponentStorageLocator CreateSimilarStorage(const ComponentStorageLocator locator)
	{
		auto& storagesVector = ComponentStorages.find(locator.ComponentType)->second;
		auto newStorage = storagesVector[locator.ComponentStorageIndex]->InitializeSimilarStorage();
		storagesVector.push_back(std::move(newStorage));
		return { locator.ComponentType, storagesVector.size() - 1 };;
	}

	bool CopyComponentToOtherStorage(const ComponentStorageLocator origin, const ComponentStorageLocator destination, const size_t componentIndex)
	{
		IComponentStorage* originInterface = ComponentStorages.find(origin.ComponentType)->second[origin.ComponentStorageIndex].get();
		IComponentStorage* destinationInterface = ComponentStorages.find(destination.ComponentType)->second[destination.ComponentStorageIndex].get();
		return originInterface->CopyComponentToOtherStorage(componentIndex, destinationInterface);
	}

	bool CopyComponentToOtherStorage(IComponentStorage* originInterface, IComponentStorage* destinationInterface, const size_t componentIndex)
	{
		return originInterface->CopyComponentToOtherStorage(componentIndex, destinationInterface);
	}


private:
	SharedBetweenManagers* WorldSharedBetweenManagers;

	std::mutex DestructiveEditMutex;

	std::map<std::type_index, std::vector<std::unique_ptr<IComponentStorage>>> ComponentStorages;
	std::map<std::type_index, std::unique_ptr<IComponentStorage>> SystemDataStorages;


	//Protection measurements against systems modifying same memory in multiple threads.
	bool ThreadingProtectionCheckOngoing = false;
	std::unordered_set<std::type_index> ThreadingProtectionReadOnlyComponentsAccessed;
	std::unordered_set<std::type_index> ThreadingProtectionWriteReadComponentsAccessed;
		
	void InitThreadingProtectionCheck()
	{
		ThreadingProtectionCheckOngoing = true;
		ThreadingProtectionReadOnlyComponentsAccessed.clear();
		ThreadingProtectionWriteReadComponentsAccessed.clear();
	}

	void FinishThreadingProtectionCheck()
	{
		ThreadingProtectionCheckOngoing = false;
	};
};
