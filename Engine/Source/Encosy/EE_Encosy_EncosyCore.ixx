module;
export module EE_Encosy_EncosyCore;

// Encosy World
export import EE_Encosy_EncosyWorld;


// Engine ECS setup
import EE_Core_SetupCoreECS;

// Core Modules
import EE_RenderCore;
import EE_Core_WindowManager;

import <memory>;
import <vector>;

export class EncosyCore
{
	friend class EngineCore;
public:
	EncosyCore() 
	{ 
		PrimaryWorld = std::make_unique<EncosyWorld>(); 
		PrimaryWorldComponentManager = PrimaryWorld->GetWorldComponentManager();
		PrimaryWorldEntityManager = PrimaryWorld->GetWorldEntityManager();
		PrimaryWorldSystemManager = PrimaryWorld->GetWorldSystemManager();
	}
	~EncosyCore() { PrimaryWorld.reset(); }

	void InitCoreSystems(RenderCore* renderCore)
	{
		EngineRenderCore = renderCore;

		EngineEntities = InitializeEngineEntities(PrimaryWorldEntityManager);
		InitializeEngineSystems(PrimaryWorldEntityManager, PrimaryWorldComponentManager, PrimaryWorldSystemManager, EngineRenderCore);

		CreateEngineEntities(PrimaryWorldEntityManager, PrimaryWorldComponentManager, PrimaryWorldSystemManager, EngineRenderCore);
	}

	void PrimaryWorldSystemManagerUpdate()
	{
		PrimaryWorldSystemManager->ManagerUpdate();
	}

	void PrimaryWorldPhysicsUpdate(const double deltaTime)
	{
		PrimaryWorldSystemManager->UpdatePhysicsSystems(deltaTime);
	}

	void PrimaryWorldSystemUpdate(const double deltaTime)
	{
		PrimaryWorldSystemManager->UpdateSystems(deltaTime);
	}

	void PrimaryWorldRenderUpdate(const double deltaTime)
	{
		PrimaryWorldSystemManager->UpdateRenderSystems(deltaTime);
	}

	void PrimaryWorldCleanup()
	{
		PrimaryWorldSystemManager->DestroySystems();
	}

	EncosyWorld* GetPrimaryWorld() { return PrimaryWorld.get(); }
	
	void Cleanup()
	{
		PrimaryWorldSystemManager->ForceStopTaskRunner();
	}

private:


	std::vector<EntityOperationResult> EngineEntities;
	std::unique_ptr<EncosyWorld> PrimaryWorld;
	RenderCore* EngineRenderCore;

	ComponentManager* PrimaryWorldComponentManager = nullptr;
	EntityManager* PrimaryWorldEntityManager = nullptr;
	SystemManager* PrimaryWorldSystemManager = nullptr;
};
