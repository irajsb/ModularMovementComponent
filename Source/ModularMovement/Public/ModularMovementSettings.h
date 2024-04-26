// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ModularMovementSettings.generated.h"

/**
 * 
 */
UCLASS(Config = Editor,DefaultConfig)
class MODULARMOVEMENT_API UModularMovementSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config)
	bool SubstepShown;
	
};
