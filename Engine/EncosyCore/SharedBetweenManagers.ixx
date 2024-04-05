module;

export module EncosyCore.SharedBetweenManagers;

import EncosyCore.Entity;

import <map>;
import <mutex>;
import <typeindex>;
import <typeinfo>;


export
class SharedBetweenManagers
{
public:
	SharedBetweenManagers(){};
	~SharedBetweenManagers()
	{
		for (auto& storageMutex : EntityStorageMutex)
		{
			delete storageMutex.second;
		}
		for (auto& storageMutex : ComponentStorageMutex)
		{
			delete storageMutex.second;
		}
	};

	size_t SignalCompositionChange()
	{
		CurrentWorldCompositionNumber_++;
		return CurrentWorldCompositionNumber_;
	}

	bool IsCurrentComposition(const size_t& number) const
	{
		return (number == CurrentWorldCompositionNumber_);
	}

	size_t GetCurrentComposition() const
	{
		return CurrentWorldCompositionNumber_;
	}

	void AddComponentStorageMutex(const std::type_index componentTypeId)
	{
		auto mutex = new std::mutex();
		ComponentStorageMutex.insert(std::make_pair(componentTypeId, mutex));
	}

	std::mutex* GetComponentStorageMutex(const std::type_index componentTypeId) 
	{
		return ComponentStorageMutex.find(componentTypeId)->second;
	}

	void AddEntityStorageMutex(const EntityType entityType)
	{
		auto mutex = new std::mutex();
		EntityStorageMutex.insert(std::make_pair(entityType, mutex));
	}

	std::mutex* GetEntityStorageMutex(const EntityType entityType) 
	{
		return EntityStorageMutex.find(entityType)->second;
	}

private:
	// Used mainly to signal other managers to rebuild critical structures
	size_t CurrentWorldCompositionNumber_ = 0;

	std::map<EntityType, std::mutex*> EntityStorageMutex;
	std::map<std::type_index, std::mutex*> ComponentStorageMutex;
};