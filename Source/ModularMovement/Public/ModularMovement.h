// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine.h"
#include "Modules/ModuleManager.h"

class FModularMovementModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};


#if UE_BUILD_SHIPPING
#define MODULAR_CYCLE_COUNTER(Stat)
#else
#define MODULAR_CYCLE_COUNTER(Stat) SCOPE_CYCLE_COUNTER(Stat)
#endif

DECLARE_STATS_GROUP(TEXT("Modular Movement"), STATGROUP_MovementPhysics, STATCAT_Advanced);

DECLARE_LOG_CATEGORY_EXTERN(LogModularVehicle, Log, All);