module;
export module EncosyEngine.EncosyCore;

// Encosy World
export import EncosyCore.EncosyWorld;


// Core System requirements
import EncosyEngine.SetupCoreECS;
import Components.TransformComponent;
import Components.MovementComponent;
import Components.CameraComponent;

//Core Modules
import EncosyEngine.RenderCore;
import EncosyEngine.WindowManager;

import <memory>;
import <vector>;

export class EncosyCore
{
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

	void PrimaryWorldPhysicsUpdate(float deltaTime)
	{
		PrimaryWorldSystemManager->UpdatePhysicsSystems(deltaTime);
	}
	void PrimaryWorldSystemUpdate(float deltaTime)
	{
		PrimaryWorldSystemManager->UpdateSystems(deltaTime);
	}
	void PrimaryWorldRenderUpdate(float deltaTime)
	{
		PrimaryWorldSystemManager->UpdateRenderSystems(deltaTime);
	}

	EncosyWorld* GetPrimaryWorld() { return PrimaryWorld.get(); }

private:
	std::vector<EntityTypeInfo> EngineEntities;
	std::unique_ptr<EncosyWorld> PrimaryWorld;
	WindowManager* EngineWindowManager;
	RenderCore* EngineRenderCore;

	ComponentManager* PrimaryWorldComponentManager = nullptr;
	EntityManager* PrimaryWorldEntityManager = nullptr;
	SystemManager* PrimaryWorldSystemManager = nullptr;
};