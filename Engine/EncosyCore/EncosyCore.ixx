module;
export module EncosyEngine.EncosyCore;

// Encosy World
export import EncosyCore.EncosyWorld;


// Engine ECS setup
import EncosyEngine.SetupCoreECS;

// Core Modules
import EncosyEngine.RenderCore;
import EncosyEngine.WindowManager;

import <memory>;
import <vector>;

export class EncosyCore
{
	friend class EncosyApplication;
public:
	EncosyCore() 
	{ 
		PrimaryWorld = std::make_unique<EncosyWorld>(); 
		PrimaryWorldComponentManager = PrimaryWorld->GetWorldComponentManager();
		PrimaryWorldEntityManager = PrimaryWorld->GetWorldEntityManager();
		PrimaryWorldSystemManager = PrimaryWorld->GetWorldSystemManager();
	}
	~EncosyCore() { PrimaryWorld.reset(); }

	void InitCoreSystems(WindowManager* windowManager, RenderCore* renderCore)
	{
		EngineWindowManager = windowManager;
		EngineRenderCore = renderCore;

		EngineEntities = InitializeEngineEntities(PrimaryWorldEntityManager);
		InitializeEngineSystems(PrimaryWorldEntityManager, PrimaryWorldComponentManager, PrimaryWorldSystemManager, EngineWindowManager, EngineRenderCore);

		CreateEngineEntities(PrimaryWorldEntityManager, PrimaryWorldComponentManager, PrimaryWorldSystemManager, EngineWindowManager, EngineRenderCore);
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

	EncosyWorld* GetPrimaryWorld() { return PrimaryWorld.get(); }

private:

	void Cleanup()
	{
		PrimaryWorldSystemManager->ForceStopTaskRunner();
	}

	std::vector<EntityOperationResult> EngineEntities;
	std::unique_ptr<EncosyWorld> PrimaryWorld;
	WindowManager* EngineWindowManager;
	RenderCore* EngineRenderCore;

	ComponentManager* PrimaryWorldComponentManager = nullptr;
	EntityManager* PrimaryWorldEntityManager = nullptr;
	SystemManager* PrimaryWorldSystemManager = nullptr;
};
