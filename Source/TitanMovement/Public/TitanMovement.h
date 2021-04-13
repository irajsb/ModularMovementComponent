// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FTitanMovementModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};


#if UE_BUILD_SHIPPING
#define ARCADE_CYCLE_COUNTER(Stat)
#else
#define ARCADE_CYCLE_COUNTER(Stat) SCOPE_CYCLE_COUNTER(Stat)
#endif

DECLARE_STATS_GROUP(TEXT("Arcade Movement"), STATGROUP_MovementPhysics, STATCAT_Advanced);

DECLARE_LOG_CATEGORY_EXTERN(LogArcadeVehicle, Log, All);